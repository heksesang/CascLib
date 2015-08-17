#pragma once

#include "CascException.hpp"

namespace Casc
{
    namespace Exceptions
    {
        class GenericException : public CascException, public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };
    }
}