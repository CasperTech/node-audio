#pragma once

#include <structs/events/CommandEvent.h>

namespace CasperTech
{
    struct PlayCommand: public CommandEvent
    {
        PlayCommand()
                : CommandEvent(Command::Play)
        {

        }
    };
}