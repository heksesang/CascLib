#pragma once

#include <array>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <vector>
#include "zlib.hpp"
#include "Endian.hpp"
#include "Hex.hpp"

namespace Casc
{
	/**
	 * A buffer which can be used to read compressed data from CASC file.
	 * When used with a stream, the stream will be able to transparently output decompressed data.
	 */
	class CascBuffer : public std::filebuf
	{
		/**
		 * The available compression modes for chunks.
		 */
		enum CompressionMode
		{
			None = 0x4E,
			Zlib = 0x5A
		};

		/**
		 * Description of a chunk.
		 */
		struct Chunk
		{
			// The offset of the first byte in the decompressed data.
			off_type begin;

			// The offset of the last byte in the decompressed data.
			off_type end;

			// The offset where the compressed data starts.
			// The base position is this->offset.
			off_type offset;

			// The size of the compressed data.
			size_t size;
		};

		// True when the file is properly initialized.
		// The file is properly initialized once all the headers have been read.
		bool isInitialized = false;

		// True if the file is currently buffering data.
		// When true, uflow will provide bytes from the actual file stream.
		// When false, uflow will provide bytes from the virtual file stream. 
		bool isBuffering = false;

		// The offset where the data starts.
		// This is where the first header is found.
		size_t offset = 0;

		// The length of the data.
		// This includes all headers.
		size_t length = 0;

		// The input buffer for zlib decompression.
		std::unique_ptr<ZStreamBase::char_t[]> in = nullptr;

		// The buffer containing the decompressed data for the current chunk.
		std::unique_ptr<ZStreamBase::char_t[]> out = nullptr;

		// The current chunk.
		Chunk currentChunk;

		// The available chunks.
		std::vector<Chunk> chunks;

		/**
		 * Read the decompressed data from a chunk into the buffer.
		 *
		 * @param chunk the chunk to read.
		 */
		void bufferChunk(Chunk &chunk)
		{
			isBuffering = true;

			out = std::unique_ptr<ZStreamBase::char_t[]>(readChunk(in.get(), chunk));

			currentChunk = chunk;

			auto beg = reinterpret_cast<char*>(out.get());
			auto end = reinterpret_cast<char*>(out.get() + (chunk.end - chunk.begin) + 1);
			auto cur = beg;

			setg(beg, cur, end);

			isBuffering = false;
		}

		/**
		 * Read the decompressed data from a chunk.
		 *
		 * @param in		the input buffer to use for zlib decompression.
		 * @param chunk		the chunk to read.
		 * @return			a smart pointer to the output byte array.
		 */
		std::unique_ptr<ZStreamBase::char_t[]> readChunk(ZStreamBase::char_t *in, Chunk& chunk)
		{
			size_t avail_in = chunk.size - 1;
			off_type offset = chunk.offset;
			size_t avail_out = 0;
			
			auto ptr = readChunk(in, avail_in, offset, avail_out);

			chunk.end = chunk.begin + avail_out - 1;
			
			return ptr;
		}

		/**
		 * Read the decompressed data from a chunk.
		 *
		 * @param in		the input buffer to use for zlib decompression.
		 * @param avail_in	the amount of available input bytes.
		 * @param offset	the offset of the beginning of data (the position of the compression mode byte).
		 * @param avail_out	reference to the output variable where the decompressed size will be written.
		 * @return			a smart pointer to the output byte array.
		 */
		std::unique_ptr<ZStreamBase::char_t[]> readChunk(ZStreamBase::char_t *in, size_t avail_in, off_type offset, size_t &avail_out)
		{
			auto pos = std::filebuf::seekpos(this->offset + offset);
			
			if (pos == std::streamoff(-1))
			{
				throw std::exception("Seek failed");
			}

			CompressionMode mode = (CompressionMode)std::filebuf::sbumpc();
			return readChunk(mode, in, avail_in, avail_out);
		}

		/**
		 * Read the decompressed data from a chunk.
		 *
		 * @param mode		the compression mode to use when reading data.
		 * @param in			the input buffer to use for zlib decompression.
		 * @param avail_in	the amount of available input bytes.
		 * @param avail_out	reference to the output variable where the decompressed size will be written.
		 * @return			a smart pointer to the output byte array.
		 */
		std::unique_ptr<ZStreamBase::char_t[]> readChunk(CompressionMode mode, ZStreamBase::char_t *in, size_t avail_in, size_t &avail_out)
		{
			if (in == nullptr)
			{
				throw std::invalid_argument("in cannot be nullptr");
			}

			ZStreamBase::char_t *out = nullptr;
			ZInflateStream stream(in, avail_in);

			switch (mode)
			{
			case 0x5A:
				std::filebuf::xsgetn(reinterpret_cast<char*>(in), avail_in);
				stream.readAll(&out, avail_out);
				break;

			case 0x4E:
				out = new ZStreamBase::char_t[avail_in];
				std::filebuf::xsgetn(reinterpret_cast<char*>(out), avail_in);
				avail_out = avail_in;
				break;

			default:
				throw std::exception("Invalid compression mode");
			}

			return std::unique_ptr<ZStreamBase::char_t[]>(out);
		}

		/**
		 * Read the header for the current file.
		 * This sets up all necessary chunk data for directly reading decompressed data.
		 */
		void readHeader()
		{
			using namespace Endian;
			char header[0x1E];
			std::filebuf::xsgetn(header, 0x1E);

			std::array<char, 16> checksum;
			std::copy(header, header + 16, checksum.begin());
			auto size = readLE<uint32_t>(header + 0x10);

			char header2[0x08];

			std::filebuf::xsgetn(header2, 0x08);
			if (readLE<uint32_t>(header2) != 0x45544C42)
			{
				throw std::exception("Not a valid BLTE file.");
			}

			auto readBytes = readBE<uint32_t>(header2 + 0x04);

			uint32_t bufferSize = 0;

			if (readBytes > 0)
			{
				readBytes -= 0x08;

				auto bytes = std::make_unique<char[]>(readBytes);
				std::filebuf::xsgetn(bytes.get(), readBytes);

				auto pos = bytes.get();

				auto flags = readBE<uint16_t>(pos);
				pos += sizeof(uint16_t);
				auto chunkCount = readBE<uint16_t>(pos);
				pos += sizeof(uint16_t);

				for (int i = 0; i < chunkCount; ++i)
				{
					auto compressedSize = readBE<uint32_t>(pos);
					pos += sizeof(uint32_t);

					bufferSize = std::max(compressedSize, bufferSize);

					auto decompressedSize = readBE<uint32_t>(pos);
					pos += sizeof(uint32_t);

					std::array<char, 16> checksum;
					std::copy(pos, pos + 16, checksum.begin());
					pos += 16;

					Chunk chunk
					{
						0,
						decompressedSize - 1,
						38 + readBytes,
						compressedSize
					};

					if (chunks.size() > 0)
					{
						auto last = chunks.back();

						chunk.begin = last.end + 1;
						chunk.end = chunk.begin + decompressedSize - 1;
						chunk.offset = last.offset + last.size;
					}

					chunks.push_back(chunk);
				}
			}
			else
			{
				chunks.push_back({
					0,
					0,
					38,
					size
				});

				bufferSize = size;
			}

			in = std::make_unique<ZStreamBase::char_t[]>(bufferSize);
		}

		/**
		 * Finds the chunk that corresponds to the given virtual offset.
		 *
		 * @param offset	the virtual offset of the decompressed data.
		 * @param out		reference to the output variable where the chunk will be written.
		 * @return			true on success, false on failure.
		 */
		bool findChunk(off_type offset, Chunk &out)
		{
			for (auto chunk : chunks)
			{
				if (chunk.begin <= offset && chunk.end >= offset)
				{
					out = chunk;
					return true;
				}
			}

			return false;
		}

		/**
		 * Finds the index of the chunk that corresponds to the given virtual offset.
		 *
		 * @param offset	the virtual offset of the decompressed data.
		 * @param out		reference to the output variable where the chunk index will be written.
		 * @return			true on success, false on failure.
		 */
		bool findChunk(off_type offset, size_t &out)
		{
			size_t i = 0;
			for (auto chunk : chunks)
			{
				if (chunk.begin <= offset && chunk.end >= offset)
				{
					out = i;
					return true;
				}

				++i;
			}

			return false;
		}

	protected:
		pos_type seekpos(pos_type pos,
			std::ios_base::openmode which = std::ios_base::in) override
		{
			return seekoff(pos, std::ios_base::beg, which);
		}

		pos_type seekoff(off_type off, std::ios_base::seekdir dir,
			std::ios_base::openmode which = std::ios_base::in) override
		{
			Chunk chunk;
			off_type offset = 0;

			switch (dir)
			{
			case std::ios_base::beg:
				offset = off;
				break;

			case std::ios_base::cur:
				offset = off + (gptr() - eback()) + currentChunk.begin;
				break;

			case std::ios_base::end:
				offset = chunks.back().end - off + 1;
				break;
			}

			if (currentChunk.begin <= offset && currentChunk.end >= offset)
			{
				setg(eback(), eback() + (offset - currentChunk.begin), egptr());
			}
			else if (findChunk(offset, chunk))
			{
				try
				{
					bufferChunk(chunk);

					setg(eback(), eback() + (offset - currentChunk.begin), egptr());
				}
				catch (std::exception&)
				{
					return pos_type(off_type(-1));
				}
			}
			else if (offset == (chunks.back().end + 1))
			{
				if (chunks.back().offset != currentChunk.offset)
				{
					try
					{
						bufferChunk(chunks.back());
					}
					catch (std::exception&)
					{
						return pos_type(off_type(-1));
					}
				}

				setg(eback(), eback() + (offset - currentChunk.begin), egptr());
			}
			else
			{
				return pos_type(off_type(-1));
			}

			return pos_type(off_type(offset));
		}

		std::streamsize showmanyc() override
		{
			return egptr() - gptr();
		}

		int_type underflow() override
		{
			Chunk chunk;

			if (findChunk(currentChunk.end + 1, chunk))
			{
				try
				{
					auto out = readChunk(in.get(), chunk);
					return std::filebuf::traits_type::to_int_type(out[0]);
				}
				catch (std::exception&)
				{
					return std::filebuf::traits_type::eof();
				}
			}
			else
			{
				return std::filebuf::traits_type::eof();
			}
		}

		int_type uflow() override
		{
			if (isInitialized && !isBuffering)
			{
				Chunk chunk;

				if (findChunk(currentChunk.end + 1, chunk))
				{
					try
					{
						bufferChunk(chunk);
					}
					catch (std::exception&)
					{
						return std::filebuf::traits_type::eof();
					}
				}
				else
				{
					return std::filebuf::traits_type::eof();
				}
			}
			
			return std::filebuf::uflow();
		}

		std::streambuf *setbuf(char_type *s, std::streamsize n) override
		{
			return this;
		}

	public:
		/**
		 * Default constructor.
		 */
		CascBuffer()
		{
		}

		/**
		 * Destructor.
		 */
		~CascBuffer() override
		{
			this->close();
		}

		/**
		 * Opens a file from the currently opened CASC file.
		 *
		 * @param offset	the offset where the file starts.
		 */
		void open(size_t offset)
		{
			this->isInitialized = false;

			if (!is_open())
			{
				throw std::exception("Buffer is not open.");
			}

			chunks.clear();

			this->offset = offset;
			std::filebuf::seekpos(offset);

			this->readHeader();

			if (chunks.size() > 0)
			{
				bufferChunk(chunks[0]);
			}

			this->isInitialized = true;
		}

		/**
		 * Opens a file in a CASC file.
		 *
		 * @param filename	the filename of the CASC file.
		 * @param offset	the offset where the file starts.
		 */
		void open(const std::string &filename, size_t offset)
		{
			if (std::filebuf::open(filename, std::ios_base::in | std::ios_base::binary) == nullptr)
			{
				throw std::exception("Couldn't open buffer.");
			}

			std::filebuf::setbuf(nullptr, 0);

			open(offset);
		}

		/**
		 * Closes the currently opened CASC file.
		 */
		void close()
		{
			this->setg(nullptr, nullptr, nullptr);

			if (std::filebuf::is_open())
			{
				std::filebuf::close();
			}

			isInitialized = false;
		}
	};
}