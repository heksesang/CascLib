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
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

#include <omp.h>

#include "../../Common.hpp"
#include "../../Exceptions.hpp"

#include "../../Parsers/Binary/Reference.hpp"

namespace Casc
{
    namespace Parsers
    {
        namespace Binary
        {
            using namespace Casc::Functions;
            using namespace Casc::Functions::Endian;
            using namespace Casc::Functions::Hash;

            /**
             * Maps file content MD5 hash to file key.
             */
            class Encoding
            {
            public:
                /**
                 * Find the file key for the given hash.
                 */
                std::vector<Hex> find(const Hex hash) const
                {
                    auto index = -1;
                    Hex checksum;

                    for (auto i = 0U; i < headersA.size(); ++i)
                    {
                        if (headersA[i].first <= hash)
                        {
                            index = headersA.size() - 1 - i;
                            checksum = headersA[i].second;
                            break;
                        }
                    }

                    if (index == -1)
                    {
                        throw Exceptions::HashDoesNotExistException(hash.string());
                    }

                    auto begin = tableA.begin() + EntrySize * index;
                    auto end = begin + EntrySize;

                    Hex actual(md5(begin, end));

                    if (actual != checksum)
                    {
                        throw Exceptions::InvalidHashException(lookup3(checksum, 0), lookup3(actual, 0), "");
                    }

                    auto files = parseEntry(begin, end, hashSizeA);

                    for (auto it = files.begin(); it != files.end(); ++it)
                    {
                        if (it->hash == hash)
                        {
                            return it->keys;
                        }
                    }
                    
                    throw Exceptions::HashDoesNotExistException(hash.string());
                }

            private:
                // The header size of an encoding file.
                static const unsigned int HeaderSize = 22U;

                // The size of each chunk body (second block for each table).
                static const unsigned int EntrySize = 4096U;

                struct FileInfo
                {
                    Hex hash;
                    size_t size;
                    std::vector<Hex> keys;

                    static const FileInfo empty;
                };

                std::vector<std::pair<Hex, Hex>> headersA;
                std::vector<char> tableA;
                size_t hashSizeA;

                struct EncodedFileInfo
                {
                    Hex hash;
                    size_t size;
                    std::string profile;
                };

                std::vector<std::pair<Hex, Hex>> headersB;
                std::vector<char> tableB;
                size_t hashSizeB;

                // The encoding profiles
                std::vector<std::string> profiles;

                /**
                * Reads data from a stream and puts it in a struct.
                */
                template <IO::EndianType Endian = IO::EndianType::Little, typename T>
                const T &read(std::shared_ptr<std::istream> stream, T &value) const
                {
                    char b[sizeof(T)];
                    stream->read(b, sizeof(T));

                    return value = Endian::read<Endian, T>(b);
                }

                /**
                 * Parse an entry in the table.
                 */
                template <typename InputIt>
                std::vector<FileInfo> parseEntry(const InputIt begin, const InputIt end, size_t hashSize) const
                {
                    std::vector<FileInfo> files;

                    for (auto it = begin; it < end;)
                    {
                        auto keyCount =
                            Endian::read<IO::EndianType::Little, uint16_t>(it);
                        it += sizeof(keyCount);

                        if (keyCount == 0)
                            break;

                        auto fileSize =
                            Endian::read<IO::EndianType::Big, uint32_t>(it);
                        it += sizeof(fileSize);

                        auto checksumIt = it;
                        it += hashSize;

                        std::vector<Hex> keys;

                        for (auto i = 0U; i < keyCount; ++i)
                        {
                            keys.emplace_back(it, it + hashSize);
                            it += hashSize;
                        }

                        files.emplace_back(FileInfo{ { checksumIt, checksumIt + hashSize }, fileSize, keys });
                    }

                    return files;
                }

                /**
                 * Parse an entry in the table.
                 */
                template <typename InputIt>
                std::vector<EncodedFileInfo> parseEncodedEntry(InputIt begin, InputIt end, size_t hashSize) const
                {
                    std::vector<EncodedFileInfo> files;

                    for (auto it = begin; it < end;)
                    {
                        auto checksumIt = it;
                        it += hashSize;

                        auto profileIndex =
                            Endian::read<IO::EndianType::Big, int32_t>(it);
                        it += sizeof(profileIndex);

                        ++it;

                        auto fileSize =
                            Endian::read<IO::EndianType::Big, uint32_t>(it);
                        it += sizeof(fileSize);

                        auto &profile = profiles[profileIndex];
                        
                        if (profileIndex >= 0)
                        {
                            files.emplace_back(EncodedFileInfo{ { checksumIt, checksumIt + hashSize }, fileSize, profile });
                        }
                        else
                        {
                            files.emplace_back(EncodedFileInfo{ { checksumIt, checksumIt + hashSize }, fileSize, "" });
                        }
                    }

                    return files;
                }

                /**
                * Parse an encoding file.
                */
                void parse(std::shared_ptr<std::istream> stream)
                {
                    uint16_t signature;
                    read<IO::EndianType::Little>(stream, signature);

                    if (signature != 0x4E45)
                    {
                        throw Exceptions::InvalidSignatureException(signature, 0x4E45);
                    }

                    // Header

                    stream->seekg(1, std::ios_base::cur); // Skip unknown

                    uint8_t hashSizeA;
                    this->hashSizeA = read<IO::EndianType::Little, uint8_t>(stream, hashSizeA);

                    uint8_t hashSizeB;
                    this->hashSizeB = read<IO::EndianType::Little, uint8_t>(stream, hashSizeB);

                    stream->seekg(4, std::ios_base::cur); // Skip flags

                    uint32_t tableSizeA;
                    read<IO::EndianType::Big>(stream, tableSizeA);

                    uint32_t tableSizeB;
                    read<IO::EndianType::Big>(stream, tableSizeB);

                    stream->seekg(1, std::ios_base::cur); // Skip unknown

                    // Encoding profiles for table B

                    uint32_t stringTableSize;
                    read<IO::EndianType::Big>(stream, stringTableSize);

                    while (stream->tellg() < (HeaderSize + stringTableSize - 1))
                    {
                        std::string profile;
                        std::getline(*stream, profile, '\0');

                        profiles.emplace_back(profile);
                    }

                    // Table A
                    for (auto i = 0U; i < tableSizeA; ++i)
                    {
                        std::vector<char> hash(hashSizeA);
                        std::vector<char> checksum(hashSizeA);

                        stream->read(hash.data(), hashSizeA);
                        stream->read(checksum.data(), hashSizeA);

                        headersA.emplace_back(std::make_pair(hash, checksum));
                    }

                    std::reverse(headersA.begin(), headersA.end());

                    tableA.resize(EntrySize * tableSizeA);
                    stream->read(tableA.data(), tableA.size());

                    // Table B

                    for (auto i = 0U; i < tableSizeB; ++i)
                    {
                        std::vector<char> hash(hashSizeA);
                        std::vector<char> checksum(hashSizeA);

                        stream->read(hash.data(), hashSizeA);
                        stream->read(checksum.data(), hashSizeA);

                        headersB.emplace_back(std::make_pair(hash, checksum));
                    }

                    std::reverse(headersB.begin(), headersB.end());

                    tableB.resize(EntrySize * tableSizeB);
                    stream->read(tableB.data(), tableB.size());

                    // Encoding profile for this file

                    std::string profile;
                    std::getline(*stream, profile, '\0');

                    profiles.emplace_back(profile);
                }

            public:
                /**
                 * Constructor.
                 */
                Encoding(Parsers::Binary::Reference ref,
                         std::shared_ptr<IO::StreamAllocator> allocator)
                {
                    parse(allocator->allocate<false>(ref));
                }

                /**
                * Copy constructor.
                */
                Encoding(const Encoding &) = default;

                /**
                 * Move constructor.
                 */
                Encoding(Encoding &&) = default;

                /**
                * Copy operator.
                */
                Encoding &operator= (const Encoding &) = default;

                /**
                 * Move operator.
                 */
                Encoding &operator= (Encoding &&) = default;

                /**
                 * Destructor.
                 */
                virtual ~Encoding() = default;

                /**
                 * Inserts a file record.
                 */
                void insert(Hex hash, Hex key, size_t fileSize)
                {
                    /*auto index = -1;
                    Hex checksum;

                    for (auto i = 0U; i < headersA.size(); ++i)
                    {
                        if (headersA[i].first <= hash)
                        {
                            index = headersA.size() - 1 - i;
                            checksum = headersA[i].second;
                            break;
                        }
                    }*/

                    std::vector<FileInfo> files;

                    for (auto index = 0U; index < headersA.size(); ++index)
                    {
                        auto size = 0;
                        auto begin = tableA.begin() + EntrySize * index;
                        auto end = begin + EntrySize;

                        for (auto it = begin; it < end;)
                        {
                            auto keyCount =
                                Endian::read<IO::EndianType::Little, uint16_t>(it);
                            it += sizeof(keyCount);

                            if (keyCount == 0)
                            {
                                size = it - begin - 4 - sizeof(keyCount);
                                break;
                            }

                            auto fileSize =
                                Endian::read<IO::EndianType::Big, uint32_t>(it);
                            it += sizeof(fileSize);

                            auto checksumIt = it;
                            it += hashSizeA;

                            std::vector<Hex> keys;

                            for (auto i = 0U; i < keyCount; ++i)
                            {
                                keys.emplace_back(it, it + hashSizeA);
                                it += hashSizeA;
                            }

                            files.emplace_back(FileInfo{ { checksumIt, checksumIt + hashSizeA }, fileSize, keys });
                        }
                    }

                    auto byteCount = 0;
                    for (auto &file : files)
                    {

                    }
                }

                /**
                 * Write the data to a stream.
                 */
                void write(std::shared_ptr<std::ofstream> stream)
                {
                    stream->close();
                }

                /**
                 * Write the data to a stream.
                 */
                void write(std::shared_ptr<IO::Stream<true>> stream)
                {
                    stream->close();
                }
            };
        }
    }
}