#pragma once

#include <string>
#include <stdint.h>
#include "CascException.hpp"

namespace Casc
{
    namespace Exceptions
    {
        class InvalidHashException : public CascException
        {
        public:
            InvalidHashException(uint32_t expected, uint32_t actual, std::string path)
                : actual(actual), expected(expected), path(path)
            {

            }

            const uint32_t actual;
            const uint32_t expected;
            const std::string path;
        };
    }
}