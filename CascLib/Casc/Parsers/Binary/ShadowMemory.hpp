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
#include <experimental/filesystem>
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

#include "Reference.hpp"

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
            private:
                static const int EntriesPerBlock = 1090U;
                static const int BlockSize = EntriesPerBlock * 5U;

                /**
                * Different SHMEM block types:
                * Header
                * Free space table
                */
                enum BlockType
                {
                    Header = 4,
                    FreeSpace = 1
                };

                // The list of versions for IDX files. Contains 16 values for WoD beta.
                std::map<uint32_t, uint32_t> versions_;

                // The list of free spaces of memory in the data files.
                std::vector<Reference> freeSpaceLength_;
                std::vector<Reference> freeSpaceOffset_;

                /**
                 * Reads a block of type BlockType::WriteableMemory.
                 */
                void readFreeSpace(std::ifstream &file)
                {
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
                 * Reads a block of type BlockType::Header.
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
                        case BlockType::Header:
                            break;

                        case BlockType::FreeSpace:
                            readFreeSpace(file);
                            break;
                        }
                    }
                }

                /**
                 * Reads a shadow memory file.
                 */
                void readFile(std::shared_ptr<std::ifstream> stream)
                {
                    uint32_t type = 0;
                    *stream >> le >> type;

                    switch (type)
                    {
                    case BlockType::Header:
                        readHeader(*stream);
                        break;

                    case BlockType::FreeSpace:
                        break;
                    }

                    stream->close();
                }

                /**
                 * Calculates how many blocks are needed for entries.
                 */
                size_t calcBlockCount(size_t count) const
                {
                    return (count % 1090) == 0 ? count / 1090 : 1 + count / 1090;
                }

            public:
                /**
                 * Constructor.
                 */
                ShadowMemory(std::shared_ptr<std::ifstream> stream)
                {
                    parse(stream);
                }

                /**
                 * Destructor.
                 */
                virtual ~ShadowMemory()
                {
                }

                /**
                 * Parses a shadow memory file.
                 */
                void parse(std::shared_ptr<std::ifstream> stream)
                {
                    readFile(stream);
                }

                /**
                 * Gets the .idx file versions.
                 */
                const std::map<uint32_t, uint32_t> &versions() const
                {
                    return versions_;
                }
            };
        }
    }
}
