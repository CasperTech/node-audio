#include "CommandException.h"

namespace CasperTech
{
    CommandException::CommandException(CommandResult result, std::string message)
            : _result(result),
              _message(std::move(message))
    {

    }

    const char* CommandException::what() const noexcept
    {
        return _message.c_str();
    }

    CommandResult CommandException::result() const
    {
        return _result;
    }

    std::string CommandException::message() const
    {
        return _message;
    }
}