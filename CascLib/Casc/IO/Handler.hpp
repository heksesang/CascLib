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

#include <fstream>
#include <memory>

#include "../zlib.hpp"
#include "../Common.hpp"

#include "EncodingMode.hpp"

namespace Casc
{
    namespace IO
    {
        /**
         * Base class for block handlers.
         *
         * Classes derived from this can be used to implement
         * different compression algorithms or encryptions.
         */
        class Handler
        {
        public:
            virtual ~Handler() { }

            /**
             * The compression mode this handler should be registered for.
             */
            virtual EncodingMode mode() const = 0;

            /**
             * Reads and processes data from the file buffer and returns the result.
             *
             * @param buf		The file buffer to read data from.
             * @param offset	The offset to read at.
             * @param inSize	The number of bytes to read from the stream.
             * @param outSize	The number of bytes to return.
             *
             * @return A pointer to the byte array.
             */
            virtual std::unique_ptr<char[]> decode(std::filebuf &buf, std::filebuf::off_type offset, size_t inSize, size_t outSize, std::filebuf::off_type &chunkSize) = 0;

            virtual std::vector<char> encode(std::istream &stream, size_t inSize) const = 0;
        };
    }
}

#include "Impl/DefaultHandler.hpp"
#include "Impl/ZlibHandler.hpp"