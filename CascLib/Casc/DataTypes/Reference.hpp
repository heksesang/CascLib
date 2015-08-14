#pragma once

#include <stdint.h>
#include <bitset>

namespace Casc
{
    namespace Shared
    {
        namespace DataTypes
        {

#pragma pack(push, 1)

            struct Ref
            {
                uint8_t hi;
                uint32_t lo;
            };

#pragma pack(pop)

        }
    }
}