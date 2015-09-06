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

#include "../../Common.hpp"

namespace Casc
{
    namespace Parsers
    {
        namespace Binary
        {
            /**
             * Contains information about which CASC files are available for writing,
             * and which is the current version of the index lists.
             */
            class ShadowMemory
            {
            public:
                Reference reserveSpace(uint32_t size)
                {
                    uint32_t available = 0;

                    for (auto it = freeSpaceLength_.begin(),
                        end = freeSpaceLength_.end(); it != end; ++it)
                    {
                        if (it->offset() > available)
                        {
                            available = it->offset();
                        }

                        if (it->offset() >= size)
                        {
                            std::vector<char> key;

                            auto location = freeSpaceOffset_.begin() + (it - freeSpaceLength_.begin());

                            Reference result(
                                key.begin(), key.end(),
                                location->file(),
                                location->offset(),
                                size);

                            if (it->offset() == size)
                            {
                                freeSpaceLength_.erase(it);
                                freeSpaceOffset_.erase(location);
                            }
                            else
                            {
                                *location = Reference(
                                    key.begin(), key.end(),
                                    location->file(),
                                    location->offset() + size,
                                    0);

                                *it = Reference(
                                    key.begin(), key.end(),
                                    0,
                                    0,
                                    it->offset() - size);
                            }


                            return result;
                        }
                    }

                    throw Exceptions::ReserveSpaceException(size, available);
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
                std::vector<Reference> freeSpaceLength_;
                std::vector<Reference> freeSpaceOffset_;

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

                    for (auto i = 0U; i < EntriesPerBlock; ++i)
                    {
                        std::array<char, 5> arr;
                        file.read(arr.data(), 5);

                        if (i < writeableMemoryCount)
                        {
                            freeSpaceLength_.emplace_back(arr.begin(), arr.end(), 0, 5, 0, 30);
                        }
                    }

                    for (auto i = 0U; i < EntriesPerBlock; ++i)
                    {
                        std::array<char, 5> arr;
                        file.read(arr.data(), 5);

                        if (i < writeableMemoryCount)
                        {
                            freeSpaceOffset_.emplace_back(arr.begin(), arr.end(), 0, 5, 0, 30);
                        }
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
                    uint32_t freeSpaceCount = freeSpaceLength_.size();

                    str << le << (uint32_t)ShmemType::FreeSpace;
                    str << le << freeSpaceCount;

                    str.seekp(0x18, std::ios_base::cur);

                    for (auto i = 0U; i < EntriesPerBlock; i++)
                    {
                        if (i < freeSpaceCount)
                        {
                            auto bytes = freeSpaceLength_.at(i).serialize(0, 5, 0, 30);
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
                            auto bytes = freeSpaceOffset_.at(i).serialize(0, 5, 0, 30);
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
                    uint32_t blockCount = calcBlockCount(freeSpaceLength_.size());

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
                ShadowMemory()
                {
                }

                /**
                 * Constructor.
                 *
                 * @param path the path of the SHMEM file.
                 * @param base the path of the base directory.
                 */
                ShadowMemory(std::string path, std::string base)
                    : path_(path)
                {
                    parse(path, base);
                }

                /**
                 * Destructor.
                 */
                virtual ~ShadowMemory()
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
            };
        }
    }
}