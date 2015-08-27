#pragma once

#include <limits>
#include <vector>

#include "Common.hpp"

namespace Casc
{
    class CascReference
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
        CascReference()
        {
        }

        template <typename KeyIt>
        CascReference(KeyIt first, KeyIt last, size_t file, size_t offset, size_t length)
            : key_(first, last), file_(file), offset_(offset), size_(length)
        {
        }

        template <typename InputIt>
        CascReference(InputIt first, InputIt last,
            size_t keySize, size_t locationSize, size_t lengthSize, size_t segmentBits)
        {
            using namespace Casc::Shared::Functions::Endian;

            auto it = first;

            std::vector<char> key(keySize);
            std::copy(it, it + keySize, key.begin());
            it += keySize;

            this->key_ = key;

            auto offsetSize = (segmentBits / 8U) + 1;
            auto fileSize = locationSize - offsetSize;

            if (fileSize > sizeof(size_t) || offsetSize > sizeof(size_t))
            {
                throw GenericException("Field size is outside the accepted range of the system.");
            }
            
            auto file = read<EndianType::Little, size_t>(it, it + fileSize);
            it += fileSize;
            auto offset = read<EndianType::Big, size_t>(it, it + offsetSize);
            it += offsetSize;
            auto size = read<EndianType::Little, size_t>(it, it + lengthSize);
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
            return{};
        }
    };
}