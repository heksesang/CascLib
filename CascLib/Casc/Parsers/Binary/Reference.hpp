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

#include <limits>
#include <vector>

#include "../../Common.hpp"
#include "../../Exceptions.hpp"

namespace Casc
{
    namespace Parsers
    {
        namespace Binary
        {
            class Reference
            {
                // The key of the referenced file.
                std::vector<char> key_;

                // The file number.
                // Max value of this field is 2^10 (10 bit).
                size_t file_ = 0;

                // The offset into the file. This is where the memory block starts.
                // Max value of this field is 2^30 (30 bit).
                size_t offset_ = 0;

                // The amount bytes in the memory block.
                // Max value of this field is 2^30 (30 bit).
                size_t size_ = 0;

            public:
                Reference()
                {
                }

                template <typename KeyIt>
                Reference(KeyIt first, KeyIt last, size_t file, size_t offset, size_t length)
                    : key_(first, last), file_(file), offset_(offset), size_(length)
                {
                }

                template <typename InputIt>
                Reference(InputIt first, InputIt last,
                    size_t keySize, size_t locationSize, size_t lengthSize, size_t segmentBits)
                {
                    using namespace Casc::Shared::Functions::Endian;

                    auto it = first;

                    std::vector<char> key(keySize);
                    std::copy(it, it + keySize, key.begin());
                    it += keySize;

                    this->key_ = key;

                    auto offsetSize = (segmentBits + 7U) / 8U;
                    auto fileSize = locationSize - offsetSize;

                    if (fileSize > sizeof(size_t) || offsetSize > sizeof(size_t))
                    {
                        throw Exceptions::ParserException("Field size is outside the accepted range of the system.");
                    }

                    auto file = read<IO::EndianType::Little, size_t>(it, it + fileSize);
                    it += fileSize;
                    auto offset = read<IO::EndianType::Big, size_t>(it, it + offsetSize);
                    it += offsetSize;
                    auto size = read<IO::EndianType::Little, size_t>(it, it + lengthSize);
                    it += lengthSize;

                    auto extraBits = (offsetSize * 8U) - segmentBits;
                    file <<= extraBits;

                    auto bits = offset >> segmentBits;
                    file |= bits;
                    offset &= (std::numeric_limits<size_t>::max() >> extraBits);

                    this->file_ = file;
                    this->offset_ = offset;
                    this->size_ = size;
                }

                const std::vector<char> &key() const
                {
                    return key_;
                }

                size_t file() const
                {
                    return file_;
                }

                size_t offset() const
                {
                    return offset_;
                }

                size_t size() const
                {
                    return size_;
                }

                std::vector<char> serialize(size_t keySize, size_t locationSize,
                    size_t lengthSize, size_t segmentBits) const
                {
                    using namespace Casc::Shared::Functions::Endian;

                    std::vector<char> v(keySize + locationSize + lengthSize);

                    if (keySize > 0)
                        std::copy(key_.begin(), key_.begin() + keySize, v.begin());

                    if (locationSize > 0)
                    {
                        auto totalBits = locationSize * 8U;

                        if (totalBits < segmentBits)
                        {
                            throw Exceptions::ParserException("Too few bits.");
                        }

                        auto fileBits = totalBits - segmentBits;
                        auto offsetBits = segmentBits;

                        std::vector<char> location(locationSize);

                        auto offsetSize = (offsetBits + 7U) / 8U;
                        auto fileSize = locationSize - offsetSize;

                        if (offset_ >= (size_t)std::pow(2, offsetBits) || file_ >= (size_t)std::pow(2, fileBits))
                        {
                            throw Exceptions::ParserException("Too few bits.");
                        }

                        auto extraBits = 0U;

                        for (auto i = 0U; i < (fileBits - fileSize * 8U); ++i)
                        {
                            extraBits = extraBits | (file_ & (size_t)std::pow(2U, i));
                        }

                        extraBits <<= offsetBits;

                        auto file = file_ >> (fileBits - fileSize * 8U);
                        auto fileBytes = write<IO::EndianType::Big>(file);
                        std::copy(fileBytes.rbegin(), fileBytes.rbegin() + fileSize,
                            v.begin() + keySize);

                        auto offset = offset_ | extraBits;
                        auto offsetBytes = write<IO::EndianType::Big>(offset);
                        std::copy(offsetBytes.begin(), offsetBytes.begin() + offsetSize,
                            v.begin() + keySize + fileSize);

                    }

                    if (lengthSize > 0)
                    {
                        auto length = write<IO::EndianType::Little>(size_);
                        std::copy(length.begin(), length.end() + lengthSize,
                            v.begin() + keySize + locationSize);
                    }

                    return v;
                }
            };
        }
    }
}