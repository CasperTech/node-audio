#pragma once

#include <structs/PlayerEvent.h>

#include <enums/Command.h>
#include <enums/CommandResult.h>

#include <functional>

namespace CasperTech
{
    struct PlayingEvent: public PlayerEvent
    {
        explicit PlayingEvent()
                : PlayerEvent(EventType::Playing)
        {

        }
    };
}