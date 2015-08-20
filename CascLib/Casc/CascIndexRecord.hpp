/*
* Copyright 2014 Gunnar Lilleaasen
*
* This file is part of CascLib.
*
* CascLib is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* CascLib is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CascLib.  If not, see <http://www.gnu.org/licenses/>.
*/

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
        typedef std::array<uint8_t, 9> key_t;

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