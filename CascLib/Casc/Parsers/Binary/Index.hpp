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

                // The path of the .idx files.
                std::string path_;

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
                std::vector<Reference> parse(const std::string &path)
                {
                    using namespace Functions::Endian;
                    using namespace Functions::Hash;

                    std::ifstream fs;
                    fs.open(path, std::ios_base::in | std::ios_base::binary);

                    uint32_t size;
                    uint32_t hash;

                    fs >> le >> size;
                    fs >> le >> hash;

                    uint32_t headerHash{ 0 };
                    if ((hash != (headerHash = lookup3(fs, size, 0))))
                    {
                        throw Exceptions::InvalidHashException(hash, headerHash, path);
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

                        dataHash = lookup3(begin, end, dataHash);
                    }

                    if (hash != dataHash.first)
                    {
                        throw Exceptions::InvalidHashException(hash, dataHash.first, path);
                    }

                    fs.seekg(0x1000 - ((8 + size) % 0x1000), std::ios_base::cur);

                    return files;
                }

                /**
                 * Write the file list to an .idx file.
                 */
                void write(std::ofstream &stream, uint32_t bucket)
                {
                    using namespace Functions;
                    using namespace Functions::Hash;

                    std::vector<char> v(24);

                    std::vector<Reference> files;

                    for (auto &file : this->files_)
                    {
                        if (findBucket(file.second.key().begin(), file.second.key().begin() + 9U) == bucket)
                        {
                            files.push_back(file.second);
                        }
                    }

                    auto version = Endian::write<IO::EndianType::Little, uint16_t>(7U);
                    auto file = Endian::write<IO::EndianType::Little, uint16_t>(bucket);
                    auto lengthSize = Endian::write<IO::EndianType::Little, uint8_t>(4U);
                    auto locationSize = Endian::write<IO::EndianType::Little, uint8_t>(5U);
                    auto keySize = Endian::write<IO::EndianType::Little, uint8_t>(9U);
                    auto segmentBits = Endian::write<IO::EndianType::Little, uint8_t>(30U);
                    auto start = Endian::write<IO::EndianType::Big, uint32_t>(0U);
                    auto end = Endian::write<IO::EndianType::Big, uint32_t>(0x40000000);

                    auto it = v.begin();

                    std::copy(version.begin(), version.end(), it);
                    std::copy(file.begin(), file.end(), it += version.size());
                    std::copy(lengthSize.begin(), lengthSize.end(), it += file.size());
                    std::copy(locationSize.begin(), locationSize.end(), it += lengthSize.size());
                    std::copy(keySize.begin(), keySize.end(), it += locationSize.size());
                    std::copy(segmentBits.begin(), segmentBits.end(), it += keySize.size());
                    std::copy(start.begin(), start.end(), it += segmentBits.size());
                    std::copy(end.begin(), end.end(), it += start.size());

                    auto headerSize = Endian::write<IO::EndianType::Little, uint32_t>(16U);
                    auto headerHash = Endian::write<IO::EndianType::Little, uint32_t>(lookup3(v.begin(), v.begin() + 16U, 0));

                    stream.write(headerSize.data(), headerSize.size());
                    stream.write(headerHash.data(), headerHash.size());
                    stream.write(v.data(), v.size());

                    auto dataSize = Endian::write<IO::EndianType::Little, uint32_t >(files.size() * 18U);
                    stream.write(dataSize.data(), dataSize.size());

                    std::pair<uint32_t, uint32_t> hash{ 0, 0 };
                    auto hashPos = stream.tellp();
                    stream.seekp(4, std::ios_base::cur);

                    std::sort(files.begin(), files.end(), [](
                        Reference &a,
                        Reference &b)
                    {
                        for (auto i = 0U; i < a.key().size(); ++i)
                        {
                            if (*reinterpret_cast<const unsigned char*>(&a.key()[i]) !=
                                *reinterpret_cast<const unsigned char*>(&b.key()[i]))
                            {
                                return *reinterpret_cast<const unsigned char*>(&b.key()[i]) >
                                    *reinterpret_cast<const unsigned char*>(&a.key()[i]);
                            }
                        }

                        return b.key() > a.key();
                    });

                    for (auto &file : files)
                    {
                        auto bytes = file.serialize(9, 5, 4, 30);
                        hash = lookup3(bytes, hash);

                        stream.write(bytes.data(), bytes.size());
                    }

                    auto pos = stream.tellp();
                    pos = pos + (0x10000 - pos % 0x10000 - 1);

                    auto dataHash = Endian::write<IO::EndianType::Little, uint32_t>(hash.first);
                    stream.seekp(hashPos);
                    stream.write(dataHash.data(), dataHash.size());

                    stream.seekp(pos); // Seek to end of reserved space.
                    stream.write("\0", 1);
                }

            public:
                /**
                 * Default constructor
                 */
                Index() { }

                /**
                 * Constructor.
                 */
                Index(const std::string path, const std::map<uint32_t, uint32_t> &versions)
                    : path_(path), versions_(versions)
                {
                    parse(path, versions);
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
                    auto result = files_.find(Functions::Hash::lookup3(first, last, 0));

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
                 * Inserts a file record.
                 */
                void insert(const Hex key, const Reference loc)
                {
                    files_[Functions::Hash::lookup3(key, 0)] = loc;
                }

                /**
                 * Parses the .idx files.
                 */
                void parse(const std::string path, const std::map<uint32_t, uint32_t> &versions)
                {
                    path_ = path;
                    versions_ = versions;

                    omp_lock_t lock;
                    omp_init_lock(&lock);

                    #pragma omp parallel for num_threads(16)
                    for (auto i = 0; i < (int)versions.size(); ++i)
                    {
                        std::stringstream ss;

                        ss << path << PathSeparator;
                        ss << std::setw(2) << std::setfill('0') << std::hex << i;
                        ss << std::setw(8) << std::setfill('0') << std::hex << versions.at(i);
                        ss << ".idx";

                        if (!fs::exists(ss.str()))
                        {
                            throw Exceptions::FileNotFoundException(ss.str());
                        }

                        auto files = parse(ss.str());
                        omp_set_lock(&lock);
                        for (auto it = files.begin(); it != files.end(); ++it)
                        {
                            files_.insert({ Functions::Hash::lookup3(it->key(), 0), *it });
                        }
                        omp_unset_lock(&lock);
                    }
                }

                /**
                 * Writes the file list back to the .idx files.
                 */
                void write()
                {
                    for (auto it = versions_.begin(); it != versions_.end(); ++it)
                    {
                        std::stringstream ss;

                        ss << path_ << PathSeparator;
                        ss << std::setw(2) << std::setfill('0') << std::hex << it->first;
                        ss << std::setw(8) << std::setfill('0') << std::hex << it->second;
                        ss << ".idx";

                        if (!fs::exists(ss.str()))
                        {
                            throw Exceptions::FileNotFoundException(ss.str());
                        }

                        std::ofstream out;
                        out.open(ss.str(), std::ios_base::out | std::ios_base::binary);
                        write(out, it->first);
                        out.close();
                    }
                }

                /**
                 * The path of the .idx files.
                 */
                const std::string &path() const
                {
                    return path_;
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