#pragma once

#include <stdint.h>
#include "CascException.hpp"

namespace Casc
{
    namespace Exceptions
    {
        class NoFreeSpaceException : public CascException
        {
        public:
            NoFreeSpaceException(uint32_t requested, uint32_t available)
                : requested(requested), available(available), CascException("Couldn't find enough free space.")
            {

            }

            const uint32_t requested;
            const uint32_t available;
        };
    }
}