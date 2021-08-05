#pragma once

#include <memory>

namespace CasperTech
{
    struct PlayerEvent;
    class IAudioPlayerEventReceiver
    {
        public:
            virtual void onPlayerEvent(const std::shared_ptr<PlayerEvent>& event) = 0;

            virtual ~IAudioPlayerEventReceiver() = default;
    };
}