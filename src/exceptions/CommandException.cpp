#include "CommandException.h"

CasperTech::CommandException::CommandException(CommandResult result, std::string message)
    : _result(result),
      _message(std::move(message))
{

}

const char* CasperTech::CommandException::what() const noexcept
{
    return _message.c_str();
}

CommandResult CasperTech::CommandException::result() const
{
    return _result;
}

std::string CasperTech::CommandException::message() const
{
    return _message;
}
