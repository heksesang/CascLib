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
#include <functional>
#include <fstream>
#include <string>

#include "Common.hpp"
#include "CascIndexRecord.hpp"

namespace Casc
{
    /**
     * Contains the index of files in the CASC archive.
     */
    class CascIndex
    {
    public:
        static uint32_t bucket(std::array<uint8_t, 9> key)
        {
            uint8_t const xorred = key[0] ^ key[1] ^ key[2] ^ key[3] ^ key[4] ^ key[5] ^ key[6] ^ key[7] ^ key[8];
            return xorred & 0xF ^ (xorred >> 4);
        }

    private:
    public:
        // The files available in the index.
        std::unordered_map<std::array<uint8_t, 9>, MemoryInfo> files;

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
            uint8_t hashFieldSize;
            uint8_t offsetBitCount;

            fs >> le >> version;
            fs >> le >> file;
            fs >> lengthFieldSize;
            fs >> locationFieldSize;
            fs >> hashFieldSize;
            fs >> offsetBitCount;

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

                CascIndexRecord *record = reinterpret_cast<CascIndexRecord*>(bytes.data());
                files[record->hash] = MemoryInfo(record->location,
                    readBE<uint32_t>(record->offset), readLE<uint32_t>(record->length));

                dataHash = lookup3(bytes, dataHash);
            }

            if (hash != dataHash.first)
            {
                throw InvalidHashException(hash, dataHash.first, path);
            }

            fs.seekg(0x1000 - ((8 + size) % 0x1000), std::ios_base::cur);
        }

        /**
         * Gets the location and size of a file.
         */
        MemoryInfo find(std::array<uint8_t, 9> key) const
        {
            auto result = files.find(key);

            if (result == files.end())
            {
                throw FileNotFoundException(Hex<9>(key).string());
            }

            return result->second;
        }

        bool insert(std::array<uint8_t, 9> key, MemoryInfo &loc)
        {
            if (bucket(key) == file)
            {
                files[key] = loc;

                return true;
            }

            return false;
        }
    };
}