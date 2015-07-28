#pragma once

#include <fstream>
#include "Shared/BufferInfo.hpp"
#include "Shared/CompressionMode.hpp"
using namespace Casc::Shared;

namespace Casc
{
	/**
	 * Base handler class for the BLTE chunks.
	 * 
	 * Classes derived from this can be used to implement
	 * different compression algorithms or encryptions.
	 */
	template <typename Traits>
    class BaseCascHandler
    {
    public:
        // Typedefs
		typedef BufferInfo<typename Traits> BufferInfo;
		typedef typename Traits::off_type off_type;

    public:
		/**
		 * The compression mode this handler should be registered for.
		 */
		virtual CompressionMode compressionMode()
		{
			return CompressionMode::None;
		}

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
        virtual std::unique_ptr<char[]> buffer(std::filebuf &buf, off_type offset, size_t inSize, size_t outSize)
        {
            return nullptr;
        }
    };

	/**
	* Default handler. This reads data directly from the stream.
	*/
	template <typename Traits = std::filebuf::traits_type>
    class DefaultHandler : public BaseCascHandler<typename Traits>
    {
        std::unique_ptr<char[]> buffer(std::filebuf &buf, off_type offset, size_t inSize, size_t outSize) override
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
    class ZlibHandler : public BaseCascHandler<typename Traits>
    {
		CompressionMode compressionMode() override
		{
			return CompressionMode::Zlib;
		}

        std::unique_ptr<char[]> buffer(std::filebuf &buf, off_type offset, size_t inSize, size_t outSize) override
        {
			ZStreamBase::char_t *out = nullptr;
			size_t avail_out;
			
			auto in = std::make_unique<ZStreamBase::char_t[]>(inSize);
			if (buf.sgetn(reinterpret_cast<char*>(in.get()), inSize) == inSize)
			{
				ZInflateStream(in.get(), inSize).readAll(&out, avail_out);

				ZStreamBase::char_t *resizedOut = new ZStreamBase::char_t[outSize];
				std::memcpy(resizedOut, out + offset, outSize);

				return std::unique_ptr<char[]>(reinterpret_cast<char*>(resizedOut));
			}

            return nullptr;
        }
    };

	typedef BaseCascHandler<std::filebuf::traits_type> CascHandler;
}