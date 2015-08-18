#pragma once

#include "CascException.hpp"

namespace Casc
{
    namespace Exceptions
    {
        class GenericException : public CascException
        {
            using CascException::CascException;
        };
    }
}