#pragma once

#include <enums/PlayerState.h>
#include <structs/events/CommandEvent.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <interfaces/IAudioPlayerEventReceiver.h>
#include "SampleRateConverter.h"
#include "VolumeFilter.h"

namespace CasperTech
{
    class FFSource;
    class RtAudioRenderer;
    class AudioPlayerImpl
    {
        public:
            explicit AudioPlayerImpl(IAudioPlayerEventReceiver* eventReceiver);

            ~AudioPlayerImpl();

            void load(const std::string& fileName, const ResultCallback& callback);
            void play(const ResultCallback& callback);
            void stop(const ResultCallback& callback);
            void seek(int64_t seekMs, const ResultCallback& callback);
            void pause(const ResultCallback& callback);
            void setVolume(float volume, const ResultCallback& callback);

        private:
            void addEvent(const std::shared_ptr<PlayerEvent>& event);
            void loadFile(const std::string& fileName);
            void handleCommand(const std::shared_ptr<CommandEvent>& command);
            void unload();
            void controlThreadFunc();
            void playThreadFunc();

            bool _running = false;
            bool _playThreadRunning = false;
            std::shared_ptr<CasperTech::RtAudioRenderer> _audioRenderer;
            std::shared_ptr<CasperTech::SampleRateConverter> _sampleRateConverter;
            std::shared_ptr<CasperTech::VolumeFilter> _volumeFilter;
            std::thread _controlThread;
            std::thread _playThread;
            std::queue<std::shared_ptr<PlayerEvent>> _eventQueue;
            std::queue<std::shared_ptr<CommandEvent>> _directPlayerCommandQueue;
            std::mutex _eventThreadMutex;
            std::mutex _playThreadMutex;
            std::mutex _directPlayerCommandMutex;
            std::condition_variable _launchWait;
            std::condition_variable _eventWait;
            std::condition_variable _playWait;
            std::condition_variable _pauseWait;

            std::shared_ptr<CasperTech::FFSource> _loadedFile;
            PlayerState _state = PlayerState::Unloaded;
            std::atomic<PlayerState> _readerState{ PlayerState::Unloaded };
            std::atomic<bool> _commandWaiting = false;
            IAudioPlayerEventReceiver* _eventReceiver;
    };
}
