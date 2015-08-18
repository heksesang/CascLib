#pragma once

#include <stdint.h>
#include "CascException.hpp"

namespace Casc
{
    namespace Exceptions
    {
        class InvalidSignatureException : public CascException
        {
        public:
            InvalidSignatureException(uint32_t actual, uint32_t expected)
                : actual(actual), expected(expected), CascException("The file signature is invalid.")
            {

            }

            const uint32_t actual;
            const uint32_t expected;
        };
    }
}