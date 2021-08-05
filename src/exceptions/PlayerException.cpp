#include "PlayerException.h"

namespace CasperTech
{
    PlayerException::PlayerException(std::string message)
            : _message(std::move(message))
    {

    }

    const char* PlayerException::what() const noexcept
    {
        return _message.c_str();
    }

    std::string PlayerException::message() const
    {
        return _message;
    }
}

