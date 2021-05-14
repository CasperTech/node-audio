#pragma once

#include <structs/PlayerCommand.h>

namespace CasperTech
{
    struct LoadCommand: public PlayerCommand
    {
        LoadCommand()
            : PlayerCommand(Command::Load)
        {

        }

        std::string fileName;
    };
}