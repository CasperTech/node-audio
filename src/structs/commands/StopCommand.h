#pragma once

#include <structs/events/CommandEvent.h>

namespace CasperTech
{
    struct StopCommand: public CommandEvent
    {
        StopCommand()
                : CommandEvent(Command::Stop)
        {

        }
    };
}