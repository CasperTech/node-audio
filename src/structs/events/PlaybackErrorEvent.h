#pragma once

#include <structs/PlayerEvent.h>

#include <enums/Command.h>
#include <enums/CommandResult.h>

#include <functional>
#include <utility>

namespace CasperTech
{
    struct PlaybackErrorEvent: public PlayerEvent
    {
        explicit PlaybackErrorEvent(int ffCode, std::string msg = "")
                : PlayerEvent(EventType::PlaybackError)
                , code(ffCode)
                , msg(std::move(msg))
        {

        }

        int code;
        std::string msg;
    };
}