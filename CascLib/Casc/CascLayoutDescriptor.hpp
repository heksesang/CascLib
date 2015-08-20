#pragma once

#include <vector>
#include "Common.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    class CascLayoutDescriptor
    {
    public:
        class ChunkDescriptor
        {
        private:
            CompressionMode mode_;
            size_t begin_;
            size_t count_;

        public:
            ChunkDescriptor(CompressionMode mode, size_t begin, size_t count)
                : mode_(mode), begin_(begin), count_(count)
            {

            }

            decltype(auto) mode() const
            {
                return mode_;
            }

            decltype(auto) begin() const
            {
                return begin_;
            }

            decltype(auto) count() const
            {
                return count_;
            }
        };

    private:
        std::vector<ChunkDescriptor> chunks_;

    public:
        CascLayoutDescriptor(std::vector<ChunkDescriptor> descriptors = {})
            : chunks_(descriptors)
        {

        }

        decltype(auto) chunks() const
        {
            return (chunks_);
        }
    };
}
