#include "AudioPlayerImpl.h"

#include <implementation/FFSource.h>

#include <structs/commands/LoadCommand.h>
#include <exceptions/CommandException.h>

namespace CasperTech
{
    AudioPlayerImpl::AudioPlayerImpl()
    {
        std::unique_lock<std::mutex> commandLock(_commandMutex);
        _controlThread = std::thread(&AudioPlayerImpl::controlThreadFunc, this);
        _launchWait.wait(commandLock, [this]
        {
            return _running;
        });
    }

    AudioPlayerImpl::~AudioPlayerImpl()
    {
        std::unique_lock<std::mutex> commandLock(_commandMutex);
        _running = false;
        _commandWait.notify_all();
        if(_controlThread.joinable())
        {
            _controlThread.join();
        }
    }

    void AudioPlayerImpl::controlThreadFunc()
    {
        {
            std::unique_lock<std::mutex> commandLock(_commandMutex);
            _running = true;
        }
        _launchWait.notify_all();

        std::shared_ptr<PlayerCommand> cmd;
        while(true)
        {
            {
                std::unique_lock<std::mutex> commandLock(_commandMutex);
                while(_running && _commandQueue.empty())
                {
                    _commandWait.wait(commandLock, [this]
                    {
                        return !_running || !_commandQueue.empty();
                    });
                }
                if(!_running)
                {
                    break;
                }
                cmd = _commandQueue.front();
                _commandQueue.pop();
            }

            if(cmd)
            {
                try
                {
                    switch(cmd->type)
                    {
                        case Command::Load:
                        {
                            auto evt = std::static_pointer_cast<LoadCommand>(cmd);
                            _loadedFile = std::make_unique<FFSource>();
                            _loadedFile->load(evt->fileName);
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
    }

    void AudioPlayerImpl::addCommand(const std::shared_ptr<PlayerCommand>& command)
    {
        std::unique_lock<std::mutex> commandLock(_commandMutex);
        _commandQueue.push(command);
        _commandWait.notify_one();
    }

    void AudioPlayerImpl::load(const std::string& fileName, const ResultCallback& callback)
    {
        auto loadCommand = std::make_shared<LoadCommand>();
        loadCommand->fileName = fileName;
        loadCommand->completionEvent = callback;
        addCommand(loadCommand);
    }
}
