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
#ifdef _MSC_VER
#include <experimental/filesystem>
#else
#include <boost/filesystem.hpp>
#endif
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#include "Common.hpp"

namespace Casc
{
    /**
     * Contains information about which CASC files are available for writing,
     * and which is the current version of the index lists.
     */
    class CascShmem
    {
    public:
        CascReference reserveSpace(uint32_t size)
        {
            uint32_t available = 0;

            for (auto& location : freeSpace_)
            {
                if (location.size() > available)
                {
                    available = location.size();
                }

                if (location.size() >= size)
                {
                    std::vector<char> key;

                    location = CascReference(
                        key.begin(), key.end(),
                        location.file(),
                        location.offset() + size,
                        location.size() - size);

                    return CascReference(
                        key.begin(), key.end(),
                        location.file(),
                        location.offset() - size,
                        size);
                }
            }

            throw NoFreeSpaceException(size, available);
        }

    private:
        typedef std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>> conv_type;

        static const int EntriesPerBlock = 1090U;
        static const int BlockSize = EntriesPerBlock * 5U;

        /**
        * Different SHMEM block types:
        * Header
        * Free space table
        */
        enum ShmemType
        {
            Header = 4,
            FreeSpace = 1
        };

        // The base directory of the archive.
        std::string basePath_;

        // The directory where the data files are stored.
        std::string dataPath_;

        // The path of the shemem file.
        std::string path_;

        // The list of versions for IDX files. Contains 16 values for WoD beta.
        std::map<uint32_t, uint32_t> versions_;

        // The list of free spaces of memory in the data files. 
        std::vector<CascReference> freeSpace_;

        /**
         * Reads a chunk of type ShmemType::WriteableMemory.
         * 
         * @param file the file stream to read the data from.
         */
        void readFreeSpace(std::ifstream &file)
        {
            using namespace Endian;
            uint32_t writeableMemoryCount;
            file >> writeableMemoryCount;

            file.seekg(24, std::ios_base::cur);

            std::array<char, 2 * BlockSize> arr;

            for (auto i = 0U; i < EntriesPerBlock; ++i)
            {
                file.read(arr.data() + i * 9, 5);
            }

            for (auto i = 0U; i < EntriesPerBlock; ++i)
            {
                file.read(arr.data() + 5 + i * 9, 4);
            }

            for (auto i = 0U; i < writeableMemoryCount; ++i)
            {
                auto begin = arr.begin() + i * 9;
                auto end = begin + 9;

                freeSpace_.emplace_back(begin, end, 0, 5, 4, 30);
            }
        }

        /**
         * Reads a chunk of type ShmemType::Header.
         *
         * @param file the file stream to read the data from.
         * @param base the path of the base directory.
         */
        void readHeader(std::ifstream &file)
        {
            uint32_t headerSize;
            file >> le >> headerSize;

            std::string path(256, '\0');
            file.read(&path[0], 256);

            int pathTypeLength = path.find_first_of(R"(\)");
            if (pathTypeLength != -1)
            {
                std::string pathType = path.substr(0, pathTypeLength);

                if (pathType.compare("Global") == 0)
                {
                    path.assign(path.substr(pathTypeLength + 1));
                }
            }

            path.resize(path.find_first_of('\0'));
            
            if (fs::path(path).is_relative())
            {
                path = fs::path(basePath_).append(path.begin(), path.end()).string();
            }

            // To make sure it works with shares mounted on linux.
            path = dataPath_ = fs::path(path_).parent_path().string();

            for (fs::directory_iterator iter(path), end; iter != end; ++iter)
            {
                if (!fs::is_directory(iter->path()))
                {
                    auto ext = iter->path().extension();
                    auto fn = iter->path().filename();
                    
                    if (ext.compare(".idx") == 0)
                    {
                        std::stringstream ss;
                        ss << std::hex << fn.string().substr(0, 2);

                        unsigned int index;
                        ss >> index;

                        versions_[index] = 0;
                    }
                }
            }

            auto blockCount = (headerSize - 264 - versions_.size() * sizeof(uint32_t)) / (sizeof(uint32_t) * 2);

            std::vector<std::pair<uint32_t, uint32_t>> blocks(blockCount);

            for (unsigned int i = 0; i < blockCount; ++i)
            {
                file >> le >> blocks[i].first;
                file >> le >> blocks[i].second;
            }

            for (unsigned int i = 0; i < versions_.size(); ++i)
            {
                file >> le >> versions_[i];
            }

            for (unsigned int i = 0; i < blockCount; ++i)
            {
                file.seekg(blocks[i].second);

                uint32_t type = 0;
                file >> le >> type;

                switch (type)
                {
                case ShmemType::Header:
                    break;

                case ShmemType::FreeSpace:
                    readFreeSpace(file);
                    break;
                }
            }
        }

        /**
         * Reads a SHMEM file.
         *
         * @param path the path of the SHMEM file.
         */
        void readFile(std::string path)
        {
            using namespace Endian;
            std::ifstream file;
            file.open(path, std::ios_base::in | std::ios_base::binary);

            uint32_t type = 0;
            file >> le >> type;

            switch (type)
            {
            case ShmemType::Header:
                readHeader(file);
                break;

            case ShmemType::FreeSpace:
                break;
            }

            file.close();
        }

        size_t calcBlockCount(size_t count) const
        {
            return (count % 1090) == 0 ? count / 1090 : 1 + count / 1090;
        }

        void writeFreeSpace(std::ofstream &str) const
        {
            uint32_t freeSpaceCount = freeSpace_.size();

            str << le << (uint32_t)ShmemType::FreeSpace;
            str << le << freeSpaceCount;

            str.seekp(0x18, std::ios_base::cur);

            for (auto i = 0U; i < EntriesPerBlock; i++)
            {
                if (i < freeSpaceCount)
                {
                    str << '\0';

                    auto bytes = freeSpace_.at(i).serialize(0, 0, 4, 0);
                    str.write(bytes.data(), bytes.size());
                }
                else
                {
                    str.write(std::array<char, 5>{ '\0', '\0', '\0', '\0', '\0' }.data(), 5);
                }
            }

            for (auto i = 0U; i < EntriesPerBlock; i++)
            {
                if (i < freeSpaceCount)
                {
                    auto bytes = freeSpace_.at(i).serialize(0, 5, 0, 30);
                    str.write(bytes.data(), bytes.size());
                }
                else
                {
                    str.write(std::array<char, 5>{ '\0', '\0', '\0', '\0', '\0' }.data(), 5);
                }
            }

            auto pos = str.tellp() % 16;

            std::array<char, 16> padding{};
            
            if (pos < 16)
            {
                str.write(padding.data() + pos, 16 - pos);
            }
        }

        void writeHeader(std::ofstream &str) const
        {
            uint32_t versionCount = versions_.size();
            uint32_t blockCount = calcBlockCount(freeSpace_.size());

            uint32_t headerSize = 264 + versionCount * sizeof(uint32_t) +
                blockCount * (sizeof(uint32_t) * 2);

            str << le << (uint32_t)ShmemType::Header;
            str << le << (uint32_t)headerSize;
            
            std::array<char, 0x100> buf{};

            std::string prefix("Global\\");
            std::string dataPath(dataPath_);
            std::replace(dataPath.begin(), dataPath.end(), '\\', '/');
            std::copy(prefix.begin(), prefix.end(), buf.begin());
            std::copy(dataPath.begin(), dataPath.end(), buf.begin() + prefix.size());

            str.write(buf.data(), 0x100);

            for (auto i = 0U; i < blockCount; ++i)
            {
                uint32_t size = 0x24 + BlockSize * 2;
                uint32_t offset = headerSize + size * i;

                if ((offset % 16) != 0)
                {
                    offset += 16 - (offset % 16);
                }

                str << le << size;
                str << le << offset;
            }

            for (auto i = 0U; i < versionCount; ++i)
            {
                str << le << versions_.at(i);
            }
        }

    public:
        void writeFile() const
        {
            std::ofstream file;
            std::string path = path_;
            file.open(path, std::ios_base::out | std::ios_base::binary);

            writeHeader(file);
            writeFreeSpace(file);
        }

    public:
        /**
         * Default constructor.
         */
        CascShmem()
        {
        }

        /**
         * Constructor.
         *
         * @param path the path of the SHMEM file.
         * @param base the path of the base directory.
         */
        CascShmem(std::string path, std::string base)
            : path_(path)
        {
            parse(path, base);
        }

        /**
         * Destructor.
         */
        virtual ~CascShmem()
        {
        }

        /**
         * Parses a SHMEM file.
         *
         * @param path the path of the SHMEM file.
         * @param base the path of the base directory.
         */
        void parse(std::string path, std::string base)
        {
            path_ = path;
            basePath_ = base;
            readFile(path);
        }

        /**
         * Gets the directory where the data files are stored.
         *
         * @return the directory path.
         */
        const std::string &path() const
        {
            return dataPath_;
        }

        /**
         * Gets the IDX file versions.
         *
         * @return the list of version numbers.
         */
        const std::map<uint32_t, uint32_t> &versions() const
        {
            return versions_;
        }

        /**
         * Gets the list of free memory blocks in CASC files.
         *
         * @return the list of free memory blocks.
         */
        const std::vector<CascReference> &freeSpace() const
        {
            return freeSpace_;
        }
    };
}