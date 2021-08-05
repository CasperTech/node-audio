#pragma once

#include <structs/events/CommandEvent.h>

namespace CasperTech
{
    struct LoadCommand: public CommandEvent
    {
        LoadCommand()
            : CommandEvent(Command::Load)
        {

        }

        std::string fileName;
    };
}