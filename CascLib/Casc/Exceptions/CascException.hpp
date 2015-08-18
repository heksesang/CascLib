#pragma once

#include <stdexcept>

namespace Casc
{
    namespace Exceptions
    {
        class CascException : public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };
    }
}