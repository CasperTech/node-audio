#pragma once

#include <structs/events/CommandEvent.h>

namespace CasperTech
{
    struct PauseCommand: public CommandEvent
    {
        PauseCommand()
                : CommandEvent(Command::Pause)
        {

        }
    };
}