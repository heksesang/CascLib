#pragma once

#include <fstream>

#include "zlib.hpp"
#include "Common.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * Base class for BLTE handlers.
     * 
     * Classes derived from this can be used to implement
     * different compression algorithms or encryptions.
     */
    template <typename Traits>
    class BaseCascBlteHandler
    {
    public:
        // Typedefs
        typedef BufferInfo<typename Traits> BufferInfo;
        typedef typename Traits::off_type off_type;

    public:
        /**
         * The compression mode this handler should be registered for.
         */
        virtual CompressionMode compressionMode() const = 0;

        /**
         * Reads and processes data from the file buffer and returns the result.
         *
         * @param buf		The file buffer to read data from.
         * @param offset	The offset to read at.
         * @param inSize	The number of bytes to read from the stream.
         * @param outSize	The number of bytes to return.
         *
         * @return A pointer to the byte array.
         */
        virtual std::unique_ptr<char[]> buffer(std::filebuf &buf, off_type offset, size_t inSize, size_t outSize, off_type &chunkSize) = 0;
    };

    /**
    * Default handler. This reads data directly from the stream.
    */
    template <typename Traits = std::filebuf::traits_type>
    class DefaultHandler : public BaseCascBlteHandler<typename Traits>
    {

        CompressionMode compressionMode() const override
        {
            return CompressionMode::None;
        }

        std::unique_ptr<char[]> buffer(std::filebuf &buf, off_type offset, size_t inSize, size_t outSize, off_type &chunkSize) override
        {
            char* out = new char[outSize];

            if (offset > 0)
                buf.pubseekoff(offset, std::ios_base::cur);
            
            if (buf.sgetn(out, outSize) == outSize)
            {
                return std::unique_ptr<char[]>(out);
            }

            return nullptr;
        }
    };

    /**
    * Zlib handler. This decompresses a zlib compressed chunk and extracts the data.
    */
    template <typename Traits = std::filebuf::traits_type>
    class ZlibHandler : public BaseCascBlteHandler<typename Traits>
    {
        ZStreamBase::char_t* out = nullptr;
        size_t avail_out = 0;

        CompressionMode compressionMode() const override
        {
            return CompressionMode::Zlib;
        }

        std::unique_ptr<char[]> buffer(std::filebuf &buf, off_type offset, size_t inSize, size_t outSize, off_type &chunkSize) override
        {
            if (offset == 0 || avail_out - offset < outSize)
            {
                auto in = std::make_unique<ZStreamBase::char_t[]>(inSize);
                if (buf.sgetn(reinterpret_cast<char*>(in.get()), inSize) == inSize)
                {
                    ZInflateStream(in.get(), inSize).readAll(&out, avail_out);
                    chunkSize = avail_out;
                }
                else
                {
                    return nullptr;
                }
            }

            ZStreamBase::char_t *resizedOut = new ZStreamBase::char_t[outSize];
            std::memcpy(resizedOut, out + offset, outSize);

            return std::unique_ptr<char[]>(reinterpret_cast<char*>(resizedOut));
        }

    public:
        virtual ~ZlibHandler()
        {
            delete[] out;
        }
    };
}