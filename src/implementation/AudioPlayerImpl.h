#pragma once

#include <structs/PlayerCommand.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#include <structs/PlayerCommand.h>

namespace CasperTech
{
    class FFSource;
    class AudioPlayerImpl
    {
        public:
            AudioPlayerImpl();

            ~AudioPlayerImpl();

            void controlThreadFunc();
            void load(const std::string& fileName, const ResultCallback& callback);

        private:
            void addCommand(const std::shared_ptr<PlayerCommand>& command);

            bool _running = false;
            std::thread _controlThread;
            std::queue<std::shared_ptr<PlayerCommand>> _commandQueue;
            std::mutex _commandMutex;
            std::condition_variable _launchWait;
            std::condition_variable _commandWait;

            std::unique_ptr<CasperTech::FFSource> _loadedFile;
    };
}

