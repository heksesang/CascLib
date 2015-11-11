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
#include <map>
#include <omp.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../Common.hpp"
#include "../../Exceptions.hpp"

#include "Reference.hpp"

namespace Casc
{
    namespace Parsers
    {
        namespace Binary
        {
            /**
             * Provides access to the file index.
             */
            class Index
            {
            private:
                // The files listed in the index.
                std::unordered_map<uint32_t, Reference> files_;

                // The versions of the .idx files.
                std::map<uint32_t, uint32_t> versions_;

                // The size of the keys in the .idx files.
                std::map<uint32_t, uint32_t> keySize_;

                /**
                 * Finds the bucket for a file key.
                 */
                template <typename KeyIt>
                uint32_t findBucket(KeyIt first, KeyIt last)
                {
                    uint8_t xorred = 0;

                    for (auto it = first; it != last; ++it)
                    {
                        xorred = xorred ^ *it;
                    }

                    return xorred & 0xF ^ (xorred >> 4);
                }

                /**
                 * Parses an .idx file.
                 */
                std::vector<Reference> parse(std::ifstream& fs)
                {
                    uint32_t size;
                    uint32_t hash;

                    fs >> le >> size;
                    fs >> le >> hash;

                    uint32_t headerHash{ 0 };
                    if ((hash != (headerHash = Crypto::lookup3(fs, size, 0))))
                    {
                        throw Exceptions::InvalidHashException(hash, headerHash, "");
                    }

                    uint16_t version;
                    uint16_t bucket;

                    uint8_t lengthFieldSize;
                    uint8_t locationFieldSize;
                    uint8_t keyFieldSize;
                    uint8_t segmentBits;

                    fs >> le >> version;
                    fs >> le >> bucket;
                    fs >> lengthFieldSize;
                    fs >> locationFieldSize;
                    fs >> keyFieldSize;
                    fs >> segmentBits;

                    this->versions_[bucket] = version;
                    this->keySize_[bucket] = keyFieldSize;

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
                    std::vector<char> data(size);

                    fs.read(data.data(), data.size());

                    std::vector<Reference> files(size / 18);
                    files.reserve(size / 18);

                    for (auto i = 0U; i < (size / 18); ++i)
                    {
                        auto begin = data.begin() + 18 * i;
                        auto end = begin + 18;

                        files.emplace_back(begin, end,
                            keyFieldSize,
                            locationFieldSize,
                            lengthFieldSize,
                            segmentBits);

                        dataHash = Crypto::lookup3(begin, end, dataHash);
                    }

                    if (hash != dataHash.first)
                    {
                        throw Exceptions::InvalidHashException(hash, dataHash.first, "");
                    }

                    fs.seekg(0xE000 - ((8 + size) % 0xD000), std::ios_base::cur);

                    return files;
                }

                /**
                 * Parses the .idx files.
                 */
                void parse(const std::map<uint32_t, uint32_t> &versions,
                    std::shared_ptr<IO::StreamAllocator> allocator)
                {
                    versions_ = versions;

                    for (auto i = 0; i < (int)versions.size(); ++i)
                    {
                        auto files = parse(*allocator->index<true, false>(i, versions.at(i)));
                        for (auto it = files.begin(); it != files.end(); ++it)
                        {
                            files_.insert({ Crypto::lookup3(it->key(), 0), *it });
                        }
                    }
                }

            public:
                /**
                 * Constructor.
                 */
                Index(const std::map<uint32_t, uint32_t> &versions,
                    std::shared_ptr<IO::StreamAllocator> allocator)
                    : versions_(versions)
                {
                    parse(versions, allocator);
                }

                /**
                 * Copy constructor.
                 */
                Index(const Index &) = default;

                /**
                 * Move constructor.
                 */
                Index(Index &&) = default;

                /**
                * Copy operator.
                */
                Index &operator= (const Index &) = default;

                /**
                 * Move operator.
                 */
                Index &operator= (Index &&) = default;

                /**
                 * Destructor
                 */
                virtual ~Index() = default;

                /**
                 * Gets a file record.
                 */
                template <typename KeyIt>
                Reference find(KeyIt first, KeyIt last) const
                {
                    auto result = files_.find(Crypto::lookup3(first, last, 0));

                    if (result == files_.end())
                    {
                        throw Exceptions::KeyDoesNotExistException(Hex(first, last).string());
                    }

                    return result->second;
                }

                /**
                * Gets a file record.
                */
                template <typename Container>
                Reference find(Container container) const
                {
                    return find(std::begin(container), std::end(container));
                }

                /**
                 * The key size for the given bucket.
                 */
                size_t keySize(uint32_t bucket) const
                {
                    return keySize_.at(bucket);
                }

                /**
                 * The bucket count.
                 */
                size_t bucketCount() const
                {
                    return versions_.size();
                }
            };
        }
    }
}