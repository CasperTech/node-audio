#pragma once
#include <exception>
#include <string>

namespace CasperTech
{
    class PlayerException: public std::exception
    {
        public:

            explicit PlayerException(std::string message);

            [[nodiscard]] const char * what () const noexcept override;
            [[nodiscard]] std::string message() const;

        private:
            std::string _message;
    };
}
