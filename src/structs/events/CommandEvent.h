#pragma once

#include <structs/PlayerEvent.h>

#include <enums/Command.h>
#include <enums/CommandResult.h>

#include <functional>

namespace CasperTech
{
    typedef std::function<void(CommandResult result, const std::string& errorMessage)> ResultCallback;

    struct CommandEvent: public PlayerEvent
    {
        explicit CommandEvent(Command type)
            : PlayerEvent(EventType::Command),
              commandType(type)
        {

        }

        Command commandType;
        ResultCallback completionEvent;
    };
}