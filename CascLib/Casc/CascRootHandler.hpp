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

#include "Common.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * Maps filename to file content MD5 hash.
     * 
     * TODO: Finish implementation, need different handling per game.
     */
    class CascRootHandler
    {
    public:
        /**
         * Find the file content hash for the given filename.
         *
         * @param filename  the filename.
         * @return          the hash in hex format.
         */
        virtual std::string findHash(std::string filename) const = 0;

    protected:
        // The container.
        std::shared_ptr<CascContainer> container;

        /**
         * Reads data from a stream and puts it in a struct.
         *
         * @param T     the type of the struct.
         * @param input the input stream.
         * @param value the output object to write the data to.
         * @param big   true if big endian.
         * @return      the data.
         */
        template <EndianType Endian, typename T>
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
                throw GenericException("Stream is in an invalid state.");
            }
        }

    public:
        /**
         * Default constructor.
         */
        CascRootHandler(std::shared_ptr<CascContainer> container)
            : container(container)
        {
            
        }

        /**
         * Move constructor.
         */
        CascRootHandler(CascRootHandler &&) = default;

        /**
        * Move operator.
        */
        CascRootHandler &operator= (CascRootHandler &&) = default;

        /**
         * Destructor.
         */
        virtual ~CascRootHandler()
        {
        }

        /**
         * The file magic of the root file.
         */
        virtual uint32_t fileMagic() const = 0;
    };
}