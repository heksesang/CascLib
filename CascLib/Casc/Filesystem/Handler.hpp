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

#include <string>
#include <memory>
#include <map>
#include <stdint.h>
#include <array>
#include <fstream>

#include "../Common.hpp"
#include "../Hex.hpp"

namespace Casc
{
    namespace Filesystem
    {
        /**
         * Maps filename to file content MD5 hash.
         *
         * TODO: Finish implementation, need different handling per game.
         */
        class Handler
        {
        public:
            /**
             * Find the file content hash for the given filename.
             */
            virtual Hex findHash(std::string path) const = 0;

        protected:
            /**
             * Reads data from a stream and puts it in a struct.
             */
            template <IO::EndianType Endian, typename T>
            const T &read(std::ifstream &stream, T &value) const
            {
                char b[sizeof(T)];
                stream.read(b, sizeof(T));

                return value = read<Endian, T>(b, b + sizeof(T));
            }

            /**
             * Throws if the fail or bad bit are set on the stream.
             */
            void checkForErrors(std::unique_ptr<std::istream>&& stream) const
            {
                if (stream->fail())
                {
                    throw Exceptions::IOException("Stream is in an invalid state.");
                }
            }

        public:
            /**
             * Default constructor.
             */
            Handler()
            {

            }

            /**
             * Move constructor.
             */
            Handler(Handler &&) = default;

            /**
            * Move operator.
            */
            Handler &operator= (Handler &&) = default;

            /**
             * Destructor.
             */
            virtual ~Handler() = default;
        };
    }
}

#include "Impl/WoWHandler.hpp"