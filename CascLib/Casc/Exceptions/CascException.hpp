#pragma once

#include <exception>

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