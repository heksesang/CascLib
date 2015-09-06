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
#include <string>
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
             * Contains the index of files in the CASC archive.
             */
            class Index
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
                typedef std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>> conv_type;

                // The files available in the index.
                std::map<std::vector<char>, Parsers::Binary::Reference> files;

                // The version of this index.
                int version;

                // The file number.
                int file;

                // The path of the index file.
                std::string path_;

                // The size of the keys in the index file.
                size_t keySize_;

            public:
                /**
                 * Constructor
                 */
                Index(const std::string path)
                    : path_(path)
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
                        throw Exceptions::InvalidHashException(hash, headerHash, path);
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
                    this->keySize_ = keyFieldSize;

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

                        Parsers::Binary::Reference ref(
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
                        throw Exceptions::InvalidHashException(hash, dataHash.first, path);
                    }

                    fs.seekg(0x1000 - ((8 + size) % 0x1000), std::ios_base::cur);
                }

                /**
                 * Bumps the version number.
                 */
                void updateVersion(int version)
                {
                    fs::path p(path_);

                    auto parent = p.parent_path();

                    std::stringstream ss;
                    conv_type conv;

                    ss << std::setw(2) << std::setfill('0') << std::hex << file;
                    ss << std::setw(8) << std::setfill('0') << std::hex << version;
                    ss << ".idx";

                    parent.append(conv.to_bytes({ fs::path::preferred_separator }));
                    parent.append(ss.str());

                    path_ = parent.string();
                }

                /**
                 * Gets a file record.
                 */
                template <typename KeyIt>
                Parsers::Binary::Reference find(KeyIt first, KeyIt last) const
                {
                    auto result = files.find({ first, last });

                    if (result == files.end())
                    {
                        throw Exceptions::KeyDoesNotExistException(Hex(first, last).string());
                    }

                    return result->second;
                }

                /**
                 * Inserts a file record.
                 */
                template <typename KeyIt>
                bool insert(KeyIt first, KeyIt last, Parsers::Binary::Reference &loc)
                {
                    if (bucket(first, last) == file)
                    {
                        files[{first, last}] = loc;

                        return true;
                    }

                    return false;
                }

                /**
                 * Write the index to file.
                 */
                void write(std::ofstream &stream)
                {
                    using namespace Hash;

                    std::vector<char> v(24);

                    auto version = Endian::write<IO::EndianType::Little, uint16_t>(7U);
                    auto file = Endian::write<IO::EndianType::Little, uint16_t>(this->file);
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

                    std::vector<std::pair<std::vector<char>, Parsers::Binary::Reference>> refs(files.begin(), files.end());

                    std::sort(refs.begin(), refs.end(), [](
                        std::pair<std::vector<char>, Parsers::Binary::Reference> &a,
                        std::pair<std::vector<char>, Parsers::Binary::Reference> &b)
                    {
                        for (auto i = 0U; i < a.first.size(); ++i)
                        {
                            if (*reinterpret_cast<unsigned char*>(&a.first[i]) != *reinterpret_cast<unsigned char*>(&b.first[i]))
                            {
                                return *reinterpret_cast<unsigned char*>(&b.first[i]) > *reinterpret_cast<unsigned char*>(&a.first[i]);
                            }
                        }

                        return b.first > a.first;
                    });

                    for (auto &file : refs)
                    {
                        auto bytes = file.second.serialize(9, 5, 4, 30);
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

                const std::string &path() const
                {
                    return path_;
                }

                const size_t &keySize() const
                {
                    return keySize_;
                }
            };
        }
    }
}