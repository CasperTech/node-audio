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
            void seek(const ResultCallback& callback, int64_t seekMs);
            void pause(const ResultCallback& callback);

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
            std::thread _controlThread;
            std::thread _playThread;
            std::queue<std::shared_ptr<PlayerEvent>> _eventQueue;
            std::mutex _eventThreadMutex;
            std::mutex _playThreadMutex;
            std::condition_variable _launchWait;
            std::condition_variable _eventWait;
            std::condition_variable _playWait;
            std::condition_variable _pauseWait;

            std::shared_ptr<CasperTech::FFSource> _loadedFile;
            PlayerState _state = PlayerState::Unloaded;
            std::atomic<PlayerState> _readerState{ PlayerState::Unloaded };
            std::atomic<int64_t> _seekTo;
            IAudioPlayerEventReceiver* _eventReceiver;
    };
}
