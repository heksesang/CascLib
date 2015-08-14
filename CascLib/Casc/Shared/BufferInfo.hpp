#pragma once

#include "ChunkInfo.hpp"

namespace Casc
{
    namespace Shared
    {
        /**
        * Description of a buffer state.
        */
        template <typename Traits>
        struct BufferInfo
        {
            // The offset of the first byte in the buffer.
            typename Traits::off_type begin;

            // The offset of the last byte in the buffer.
            typename Traits::off_type end;

            // The offset of the current byte in the buffer.
            typename Traits::off_type offset;
        };
    }
}