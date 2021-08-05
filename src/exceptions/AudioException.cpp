#include "AudioException.h"

namespace CasperTech
{
    AudioException::AudioException(AudioError result, std::string message)
            : _result(result),
              _message(std::move(message))
    {

    }

    const char* AudioException::what() const noexcept
    {
        return _message.c_str();
    }

    AudioError AudioException::result() const
    {
        return _result;
    }

    std::string AudioException::message() const
    {
        return _message;
    }
}