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
#include "../md5.hpp"

#include "Chunk.hpp"
#include "DataSource.hpp"
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
        protected:
            std::shared_ptr<DataSource> source;

        public:
            /**
             * Constructor.
             */
            class Handler(Chunk chunk, std::shared_ptr<DataSource> source)
                : chunk(chunk), source(source) { }

            /**
             * Destructor.
             */
            virtual ~Handler() { }

            /**
             * What kind of encoding this handler supports.
             */
            virtual EncodingMode mode() const = 0;

            /**
             * Decodes a chunk of data.
             */
            virtual std::vector<char> decode(size_t offset, size_t count) = 0;

            /**
             * Encodes data from the stream and returns the result.
             */
            virtual std::vector<char> encode(std::vector<char> input) const = 0;

            /**
             * Returns the logical, decoded size of the chunk.
             */
            virtual size_t logicalSize() = 0;

            /**
             * Clears the buffers.
             */
            virtual void reset() = 0;

            /**
             * Checks the data against the MD5 checksum.
             */
            bool validate()
            {
                auto data = this->source->get(1, SIZE_MAX);
                auto hash = Hex(md5(data));

                return hash == chunk.checksum;
            }
            
            /**
             * Chunk metadata.
             */
            const Chunk chunk;
        };
    }
}

#include "Impl/NoneHandler.hpp"
#include "Impl/ZlibHandler.hpp"
#include "Impl/CryptHandler.hpp"