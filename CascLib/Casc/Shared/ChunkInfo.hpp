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
            Traits::off_type begin;

            // The offset of the last byte in the decompressed data.
            Traits::off_type end;

            // The offset where the compressed data starts.
            // The base position is this->offset.
            Traits::off_type offset;

            // The size of the compressed data.
            size_t size;

            // Data is compressed.
            /*bool compressed;*/
        };
    }
}