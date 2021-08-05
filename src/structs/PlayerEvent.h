#pragma once

#include <enums/EventType.h>

#include <functional>

namespace CasperTech
{
    struct PlayerEvent
    {
        explicit PlayerEvent(EventType eventType)
                : eventType(eventType)
        {

        }

        EventType eventType;
    };
}