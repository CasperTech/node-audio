#pragma once

#include <enums/AudioError.h>

#include <exception>
#include <string>

namespace CasperTech
{
    class AudioException: public std::exception
    {
        public:

            AudioException(AudioError result, std::string message);

            [[nodiscard]] const char * what () const noexcept override;
            [[nodiscard]] AudioError result() const;
            [[nodiscard]] std::string message() const;

        private:
            AudioError _result;
            std::string _message;
    };
}
