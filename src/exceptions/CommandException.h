#pragma once

#include <enums/CommandResult.h>

#include <exception>
#include <string>

namespace CasperTech
{
    class CommandException: public std::exception
    {
        public:

            CommandException(CommandResult result, std::string message);

            [[nodiscard]] const char * what () const noexcept override;
            [[nodiscard]] CommandResult result() const;
            [[nodiscard]] std::string message() const;

        private:
            CommandResult _result;
            std::string _message;
    };
}
