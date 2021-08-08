#pragma once

#include <structs/events/CommandEvent.h>

namespace CasperTech
{
    struct SetVolumeCommand: public CommandEvent
    {
        SetVolumeCommand()
                : CommandEvent(Command::SetVolume)
        {

        }

        float volume = 1.0;
    };
}