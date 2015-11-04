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
         * Base block handler.
         */
        class Handler
        {
        public:
            virtual ~Handler() { }

            /**
             * The encoding mode of the handler.
             */
            virtual EncodingMode mode() const = 0;

            /**
             * Decodes data from the filebuf and returns the result.
             */
            virtual std::unique_ptr<char[]> decode(std::filebuf &buf, std::filebuf::off_type offset, size_t inSize, size_t outSize) = 0;

            /**
             * Encodes data from the stream and returns the result.
             */
            virtual std::vector<char> encode(std::vector<char> input) const = 0;
        };
    }
}

#include "Impl/DefaultHandler.hpp"
#include "Impl/ZlibHandler.hpp"