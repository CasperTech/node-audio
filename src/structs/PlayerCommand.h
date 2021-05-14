#pragma once

#include <enums/Command.h>
#include <enums/CommandResult.h>

#include <functional>

namespace CasperTech
{
    typedef std::function<void(CommandResult result, const std::string& errorMessage)> ResultCallback;

    struct PlayerCommand
    {
        explicit PlayerCommand(Command type)
            : type(type)
        {

        }

        Command type;
        ResultCallback completionEvent;
    };
}