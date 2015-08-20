#pragma once

namespace Casc
{
    namespace Shared
    {
        /**
        * Description of a chunk.
        */
        struct ChunkInfo
        {
            typedef std::filebuf::traits_type Traits;

            // The offset of the first byte in the decompressed data.
            typename Traits::off_type begin;

            // The offset of the last byte in the decompressed data.
            typename Traits::off_type end;

            // The offset where the compressed data starts.
            // The base position is this->offset.
            typename Traits::off_type offset;

            // The size of the compressed data.
            size_t size;

            // Data is compressed.
            /*bool compressed;*/
        };
    }
}