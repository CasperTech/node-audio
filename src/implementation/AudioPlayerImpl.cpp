#include "AudioPlayerImpl.h"
#include "VolumeFilter.h"

#include <implementation/FFSource.h>
#include <implementation/FFFrame.h>
#include <implementation/RtAudioRenderer.h>

#include <structs/commands/LoadCommand.h>
#include <structs/commands/PlayCommand.h>
#include <structs/commands/StopCommand.h>
#include <structs/commands/SeekCommand.h>
#include <structs/commands/PauseCommand.h>
#include <structs/commands/SetVolumeCommand.h>
#include <structs/events/PlaybackFinishedEvent.h>

#include <exceptions/CommandException.h>

#include <cassert>
#include <thread>
#include <iostream>
#include <structs/events/PlaybackErrorEvent.h>
#include <structs/events/PlayingEvent.h>
#include <exceptions/AudioException.h>

namespace CasperTech
{
    AudioPlayerImpl::AudioPlayerImpl(IAudioPlayerEventReceiver* eventReceiver)
        : _eventReceiver(eventReceiver)
        , _audioRenderer(std::make_shared<RtAudioRenderer>())
        , _volumeFilter(std::make_shared<VolumeFilter>())
        , _sampleRateConverter(std::make_shared<SampleRateConverter>())
    {
        std::unique_lock<std::mutex> commandLock(_eventThreadMutex);
        _controlThread = std::thread(&AudioPlayerImpl::controlThreadFunc, this);
        _launchWait.wait(commandLock, [this]
        {
            return _running;
        });
    }

    AudioPlayerImpl::~AudioPlayerImpl()
    {
#ifdef _DEBUG
        std::cout << "Destructing AudiPlayerImpl" << std::endl;
#endif
        {
            std::unique_lock<std::mutex> commandLock(_eventThreadMutex);
            _running = false;
            _eventWait.notify_all();
        }
        bool unpause = false;
        {
            std::unique_lock<std::mutex> commandLock(_playThreadMutex);
            if (_readerState == PlayerState::Paused)
            {
                unpause = true;
            }
            _readerState = PlayerState::Unloaded;
        }
        if (unpause)
        {
            _pauseWait.notify_all();
        }
        if (_playThread.joinable())
        {
            _playThread.join();
            _playThreadRunning = false;
        }

        if (_audioRenderer)
        {
            _audioRenderer.reset();
        }
        if (_sampleRateConverter)
        {
            _sampleRateConverter->disconnectSink();
            _sampleRateConverter.reset();
        }
        if (_volumeFilter)
        {
            _volumeFilter->disconnectSink();
            _volumeFilter.reset();
        }
        if (_loadedFile)
        {
            _loadedFile->disconnectSink();
            _loadedFile.reset();
        }

        if(_controlThread.joinable())
        {
            _controlThread.join();
        }
#ifdef _DEBUG
        std::cout << "Done destructing AudioPlayerIMpl" << std::endl;
#endif
    }

    void AudioPlayerImpl::controlThreadFunc()
    {
        {
            std::unique_lock<std::mutex> commandLock(_eventThreadMutex);
            _running = true;
        }
        _launchWait.notify_all();

        std::shared_ptr<PlayerEvent> evt;
        while(true)
        {
            {
                std::unique_lock<std::mutex> commandLock(_eventThreadMutex);
                while(_running && _eventQueue.empty())
                {
                    _eventWait.wait(commandLock, [this]
                    {
                        return !_running || !_eventQueue.empty();
                    });
                }
                if(!_running)
                {
                    break;
                }
                evt = _eventQueue.front();
                _eventQueue.pop();
            }

            if(evt)
            {
                try
                {
                    switch(evt->eventType)
                    {
                        case EventType::Command:
                        {
                            auto cmd = std::static_pointer_cast<CommandEvent>(evt);
                            handleCommand(cmd);
                            break;
                        }
                        case EventType::PlaybackFinished:
                        {
                            std::unique_lock<std::mutex> lk(_playThreadMutex);
                            
                            _readerState = PlayerState::Paused;
                            _state = PlayerState::Paused;
                            break;
                        }
                        case EventType::PlaybackError:
                        {
                            auto errorEvt = std::static_pointer_cast<PlaybackErrorEvent>(evt);
                            errorEvt->msg = FFSource::getError(errorEvt->code);
                            break;
                        }
                    }

                    _eventReceiver->onPlayerEvent(evt);
                }
                catch(...)
                {
                    // Event error, assert but dont stop processing
                    assert(false);
                }
            }
        }
    }

    void AudioPlayerImpl::addEvent(const std::shared_ptr<PlayerEvent>& command)
    {
        std::unique_lock<std::mutex> eventLock(_eventThreadMutex);
        _eventQueue.push(command);
        _eventWait.notify_one();
    }

    void AudioPlayerImpl::load(const std::string& fileName, const ResultCallback& callback)
    {
        auto loadCommand = std::make_shared<LoadCommand>();
        loadCommand->fileName = fileName;
        loadCommand->completionEvent = callback;
        addEvent(loadCommand);
    }

    void AudioPlayerImpl::loadFile(const std::string& fileName)
    {
        if(_state != PlayerState::Unloaded)
        {
            unload();
        }
        _loadedFile = std::make_shared<FFSource>();
        _loadedFile->load(fileName);
        _state = PlayerState::Loaded;
    }

    void AudioPlayerImpl::unload()
    {
        _state = PlayerState::Unloaded;
    }

    void AudioPlayerImpl::play(const ResultCallback& callback)
    {
        if(_state == PlayerState::Unloaded)
        {
            throw CommandException(CommandResult::GenericFailure, "No file loaded");
        }
        if(_state == PlayerState::Playing)
        {
            return;
        }

        auto playCommand = std::make_shared<PlayCommand>();
        playCommand->completionEvent = callback;
        addEvent(playCommand);
    }

    void AudioPlayerImpl::stop(const ResultCallback& callback)
    {
        auto stopCommand = std::make_shared<StopCommand>();
        stopCommand->completionEvent = callback;
        addEvent(stopCommand);
    }

    void AudioPlayerImpl::pause(const ResultCallback& callback)
    {
        auto pauseCommand = std::make_shared<PauseCommand>();
        pauseCommand->completionEvent = callback;
        addEvent(pauseCommand);
    }

    void AudioPlayerImpl::seek(int64_t seekMs, const ResultCallback& callback)
    {
        auto seekCommand = std::make_shared<SeekCommand>();
        seekCommand->completionEvent = callback;
        seekCommand->seekMs = seekMs;
        addEvent(seekCommand);
    }

    void AudioPlayerImpl::setVolume(float volume, const ResultCallback& callback)
    {
        auto setVolumeCommand = std::make_shared<SetVolumeCommand>();
        setVolumeCommand->completionEvent = callback;
        setVolumeCommand->volume = volume;
        addEvent(setVolumeCommand);
    }

    void AudioPlayerImpl::playThreadFunc()
    {
        {
            std::unique_lock<std::mutex> commandLock(_playThreadMutex);
            _playThreadRunning = true;
        }
        _playWait.notify_all();

        std::unique_ptr<FFFrame> frame = std::make_unique<FFFrame>();
        int result = 1;
        addEvent(std::make_shared<PlayingEvent>());
        while(_readerState > PlayerState::Loaded)
        {
            if (_readerState == PlayerState::Paused)
            {
                std::unique_lock<std::mutex> lk(_playThreadMutex);
                _pauseWait.wait(lk, [this]
                {
                    return _readerState != PlayerState::Paused;
                });
                continue;
            }
            if (_commandWaiting)
            {
                {
                    std::unique_lock<std::mutex> lk(_directPlayerCommandMutex);
                    while(!_directPlayerCommandQueue.empty())
                    {
                        auto evt = _directPlayerCommandQueue.front();
                        _directPlayerCommandQueue.pop();
                        switch(evt->commandType)
                        {
                            case Command::Seek:
                            {
                                auto event = std::static_pointer_cast<SeekCommand>(evt);
                                _loadedFile->seek(event->seekMs);
                                break;
                            }
                            case Command::SetVolume:
                            {
                                auto event = std::static_pointer_cast<SetVolumeCommand>(evt);
                                _volumeFilter->setVolume(event->volume);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    _commandWaiting = false;
                }
            }
            result = _loadedFile->getPacket(frame.get());
            if (result == -11)
            {
                result = 1;
                continue;
            }
            if (result == 0)
            {
                _readerState = PlayerState::Paused;
                addEvent(std::make_shared<PlaybackFinishedEvent>());
            }
        }
        if (result < 0)
        {
            addEvent(std::make_shared<PlaybackErrorEvent>(result));
            addEvent(std::make_shared<PlaybackFinishedEvent>());
            return;
        }
        _loadedFile->eos();
        _commandWaiting = false;
    }

    void AudioPlayerImpl::handleCommand(const std::shared_ptr<CommandEvent>& cmd)
    {
        try
        {
            switch(cmd->commandType)
            {
                case Command::Load:
                {
                    // First unload and stop any existing thread

                    bool unpause = false;
                    bool join = false;
                    {
                        std::unique_lock<std::mutex> lk(_playThreadMutex);
                        if(_playThreadRunning)
                        {
                            join = true;
                            if(_readerState == PlayerState::Paused)
                            {
                                unpause = true;
                            }
                            _readerState = PlayerState::Unloaded;
                        }
                    }
                    if(unpause)
                    {
                        _pauseWait.notify_all();
                    }
                    if(join && _playThread.joinable())
                    {
                        _playThread.join();
                        _playThreadRunning = false;
                    }

                    // Load the new file
                    auto evt = std::static_pointer_cast<LoadCommand>(cmd);
                    loadFile(evt->fileName);

                    // Spawn the play thread
                    {
                        std::unique_lock<std::mutex> lk(_playThreadMutex);
                        try
                        {
                            _sampleRateConverter->connectSink(_audioRenderer);
                            //_loadedFile->connectSink(_sampleRateConverter);
                            _volumeFilter->connectSink(_sampleRateConverter);
                            _loadedFile->connectSink(_volumeFilter);
                        }
                        catch(const AudioException& e)
                        {
                            lk.unlock();
                            evt->completionEvent(CommandResult::PlayError, e.message());
                            return;
                        }
                        // Start in a paused state
                        _readerState = PlayerState::Paused;
                        _playThread = std::thread(&AudioPlayerImpl::playThreadFunc, this);
                        _playWait.wait(lk, [this]
                        {
                            return _playThreadRunning;
                        });
                    }
                    evt->completionEvent(CommandResult::Success, "");
                    break;
                }
                case Command::Play:
                {
                    auto evt = std::static_pointer_cast<PlayCommand>(cmd);
                    {
                        if(!_loadedFile)
                        {
                            throw CommandException(CommandResult::GenericFailure, "No file loaded");
                        }
                        bool unpause = false;
                        {
                            std::unique_lock<std::mutex> lk(_playThreadMutex);
                            if(!_playThreadRunning)
                            {
                                throw CommandException(CommandResult::GenericFailure, "No file playing");
                            }
                            if (_readerState == PlayerState::Paused)
                            {
                                unpause = true;
                                _readerState = PlayerState::Playing;
                            }
                        }
                        if (unpause)
                        {
                            _pauseWait.notify_all();
                        }
                        evt->completionEvent(CommandResult::Success, "");
                    }
                    break;
                }
                case Command::Stop:
                {
                    auto evt = std::static_pointer_cast<StopCommand>(cmd);
                    {
                        if(!_loadedFile)
                        {
                            throw CommandException(CommandResult::GenericFailure, "No file loaded");
                        }
                        bool unpause = false;
                        {
                            std::unique_lock<std::mutex> commandLock(_playThreadMutex);
                            if (_readerState == PlayerState::Paused)
                            {
                                unpause = true;
                            }
                            _readerState = PlayerState::Unloaded;
                        }
                        if (unpause)
                        {
                            _pauseWait.notify_all();
                        }
                        if(_playThread.joinable())
                        {
                            _playThread.join();
                            _playThreadRunning = false;
                        }


                        _audioRenderer.reset();
                        _sampleRateConverter->disconnectSink();
                        _sampleRateConverter.reset();
                        _volumeFilter->disconnectSink();
                        _volumeFilter.reset();
                        _loadedFile->disconnectSink();
                        _loadedFile.reset();

                        _audioRenderer = std::make_shared<RtAudioRenderer>();
                        _sampleRateConverter = std::make_shared<SampleRateConverter>();
                        _volumeFilter = std::make_shared<VolumeFilter>();

                        evt->completionEvent(CommandResult::Success, "");
                    }
                    break;
                }
                case Command::Pause:
                {
                    auto evt = std::static_pointer_cast<StopCommand>(cmd);
                    if(!_loadedFile)
                    {
                        throw CommandException(CommandResult::GenericFailure, "No file loaded");
                    }
                    {
                        std::unique_lock<std::mutex> commandLock(_playThreadMutex);
                        _readerState = PlayerState::Paused;
                    }
                    evt->completionEvent(CommandResult::Success, "");
                    break;
                }
                case Command::SetVolume:
                {
                    auto evt = std::static_pointer_cast<SetVolumeCommand>(cmd);
                    std::unique_lock<std::mutex> lk(_directPlayerCommandMutex);
                    _directPlayerCommandQueue.push(cmd);
                    _commandWaiting = true;
                    evt->completionEvent(CommandResult::Success, "");
                    break;
                }
                case Command::Seek:
                {
                    auto evt = std::static_pointer_cast<SeekCommand>(cmd);
                    std::unique_lock<std::mutex> lk(_directPlayerCommandMutex);
                    _directPlayerCommandQueue.push(cmd);
                    _commandWaiting = true;
                    evt->completionEvent(CommandResult::Success, "");
                    break;
                }
                case Command::None:
                    [[fallthrough]];
                default:
                    break;
            }
        }
        catch(const CommandException& e)
        {
            cmd->completionEvent(e.result(), e.message());
        }
    }
}
