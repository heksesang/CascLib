#pragma once

namespace Casc
{
    namespace Shared
    {
        /**
        * Description of a buffer state.
        */
        struct BufferInfo
        {
            typedef std::filebuf::traits_type Traits;

            // The offset of the first byte in the buffer.
            typename Traits::off_type begin;

            // The offset of the last byte in the buffer.
            typename Traits::off_type end;

            // The offset of the current byte in the buffer.
            typename Traits::off_type offset;
        };
    }
}