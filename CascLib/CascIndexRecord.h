#pragma once

#include <array>
#include <stdint.h>

namespace Casc
{
#pragma pack(push, 1)
    
    /**
    * A file record in the index of files in the CASC archive.
    */
    struct CascIndexRecord
    {
    public:
        // Typedefs
        typedef std::array<char, 9> key_t;

    public:
        // The file key.
        key_t hash;

        // The data file where the file is located.
        uint8_t location;

        // The offset into the data file where the file starts.
        uint32_t offset;

        // The size of the file.
        uint32_t length;
    };

#pragma pack(pop)
}