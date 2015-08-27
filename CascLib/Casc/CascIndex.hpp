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

#include <array>
#include <functional>
#include <fstream>
#include <string>

#include "Common.hpp"

namespace Casc
{
    /**
     * Contains the index of files in the CASC archive.
     */
    class CascIndex
    {
    public:
        /**
         * Gets the bucket number for a file key.
         */
        template <typename KeyIt>
        static uint32_t bucket(KeyIt first, KeyIt last)
        {
            uint8_t xorred = 0;

            for (auto it = first; it != last; ++it)
            {
                if ((it - last) > 1)
                    xorred = xorred ^ *it;
            }

            return xorred & 0xF ^ (xorred >> 4);
        }

    private:
    public:
        // The files available in the index.
        std::unordered_map<std::vector<char>, CascReference> files;

        // The version of this index.
        int version;

        // The file number.
        int file;

        // The path of the index file.
        std::string path;

    public:
        /**
         * Constructor
         */
        CascIndex(std::string path)
            : path(path)
        {
            using namespace Endian;
            using namespace Hash;
            std::ifstream fs;
            fs.open(path, std::ios_base::in | std::ios_base::binary);

            uint32_t size;
            uint32_t hash;

            fs >> le >> size;
            fs >> le >> hash;

            uint32_t headerHash{ 0 };
            if ((hash != (headerHash = lookup3(fs, size, 0))))
            {
                throw InvalidHashException(hash, headerHash, path);
            }

            uint16_t version;
            uint16_t file;
            
            uint8_t lengthFieldSize;
            uint8_t locationFieldSize;
            uint8_t keyFieldSize;
            uint8_t segmentBits;

            fs >> le >> version;
            fs >> le >> file;
            fs >> lengthFieldSize;
            fs >> locationFieldSize;
            fs >> keyFieldSize;
            fs >> segmentBits;

            this->version = version;
            this->file = file;

            for (unsigned int i = 0; i < (size - 8); i += 8)
            {
                uint32_t dataBeg;
                uint32_t dataEnd;

                fs >> be >> dataBeg;
                fs >> be >> dataEnd;
            }

            fs.seekg(16 - ((8 + size) % 16), std::ios_base::cur);

            fs >> le >> size;
            fs >> le >> hash;

            std::pair<uint32_t, uint32_t> dataHash{ 0, 0 };
            for (auto i = 0U; i < (size / 18); ++i)
            {
                std::array<char, 18> bytes{};
                fs.read(bytes.data(), 18);

                CascReference ref(
                    bytes.begin(), bytes.end(),
                    keyFieldSize,
                    locationFieldSize,
                    lengthFieldSize,
                    segmentBits);

                files.insert({ ref.key(), ref });

                dataHash = lookup3(bytes, dataHash);
            }

            if (hash != dataHash.first)
            {
                throw InvalidHashException(hash, dataHash.first, path);
            }

            fs.seekg(0x1000 - ((8 + size) % 0x1000), std::ios_base::cur);
        }

        /**
         * Gets a file record.
         */
        template <typename KeyIt>
        CascReference find(KeyIt first, KeyIt last) const
        {
            auto result = files.find({ first, last });

            if (result == files.end())
            {
                throw FileNotFoundException(Hex(first, last).string());
            }

            return result->second;
        }

        /**
         * Inserts a file record.
         */
        template <typename KeyIt>
        bool insert(KeyIt first, KeyIt last, CascReference &loc)
        {
            if (bucket(first, last) == file)
            {
                files[{first, last}] = loc;

                return true;
            }

            return false;
        }
    };
}