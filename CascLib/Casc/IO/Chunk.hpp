/*
* Copyright 2015 Gunnar Lilleaasen
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

#include "../Hex.hpp"

namespace Casc
{
    namespace IO
    {
        struct Chunk
        {
            // The offset of the first byte in the decompressed data.
            size_t begin;

            // The offset of the last byte in the decompressed data.
            size_t end;

            // The offset where the compressed data starts.
            size_t offset;

            // The size of the compressed data.
            size_t size;

            // The checksum of the data.
            Hex checksum;

            bool operator <(const Chunk &b) const
            {
                return this->begin < b.begin;
            }
            bool operator >(const Chunk &b) const
            {
                return this->begin > b.begin;
            }
        };
    }
}