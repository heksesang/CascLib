#pragma once

namespace Casc
{
    namespace Shared
    {
        /**
        * The available compression modes for chunks.
        */
        enum CompressionMode
        {
            None = 0x4E,
            Zlib = 0x5A
        };
    }
}