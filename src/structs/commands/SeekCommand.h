#pragma once

#include <structs/events/CommandEvent.h>

namespace CasperTech
{
    struct SeekCommand: public CommandEvent
    {
        SeekCommand()
                : CommandEvent(Command::Seek)
        {

        }

        int64_t seekMs = 0;
    };
}