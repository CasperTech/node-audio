#pragma once

#include <structs/PlayerEvent.h>

#include <enums/Command.h>
#include <enums/CommandResult.h>

#include <functional>

namespace CasperTech
{
    struct PlaybackFinishedEvent: public PlayerEvent
    {
        explicit PlaybackFinishedEvent()
                : PlayerEvent(EventType::PlaybackFinished)
        {

        }
    };
}