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
                /**
                 * Default constructor.
                 */
                Reference() { }

                /**
                 * Constructor.
                 */
                template <typename KeyIt>
                Reference(KeyIt first, KeyIt last, size_t file, size_t offset, size_t length)
                    : key_(first, last), file_(file), offset_(offset), size_(length)
                {
                }

                /**
                 * Constructor.
                 */
                template <typename InputIt>
                Reference(InputIt first, InputIt last,
                    size_t keySize, size_t locationSize, size_t lengthSize, size_t segmentBits)
                {
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

                    auto file = IO::Endian::read<IO::EndianType::Little, size_t>(it, it + fileSize);
                    it += fileSize;
                    auto offset = IO::Endian::read<IO::EndianType::Big, size_t>(it, it + offsetSize);
                    it += offsetSize;
                    auto size = IO::Endian::read<IO::EndianType::Little, size_t>(it, it + lengthSize);
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

                /**
                 * Copy constructor.
                 */
                Reference(const Reference &) = default;

                /**
                 * Move constructor.
                 */
                Reference(Reference &&) = default;

                /**
                 * Copy operator.
                 */
                Reference &operator= (const Reference &) = default;

                /**
                 * Move operator.
                 */
                Reference &operator= (Reference &&) = default;

                /**
                 * Destructor
                 */
                virtual ~Reference() = default;

                /**
                 * The key.
                 */
                const std::vector<char> &key() const
                {
                    return key_;
                }

                /**
                 * The file number.
                 */
                size_t file() const
                {
                    return file_;
                }

                /**
                 * The offset.
                 */
                size_t offset() const
                {
                    return offset_;
                }

                /**
                 * The size.
                 */
                size_t size() const
                {
                    return size_;
                }
            };
        }
    }
}