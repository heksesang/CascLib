#pragma once

#include <stdexcept>

namespace Casc
{
    namespace Exceptions
    {
        class CascException : public std::exception
        {
            using std::exception::exception;
        };
    }
}