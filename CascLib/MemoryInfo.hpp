#pragma once

#include <bitset>
#include <stdint.h>

namespace Casc
{
	/// Data structure that contains information about a
	/// block of memory in the data files.
	class MemoryInfo
	{
	public:
		/// The file number.
		/// Max value of this field is 2^10 (10 bit).
		int file_ = 0;

		/// The offset into the file. This is where the memory block starts.
		/// Max value of this field is 2^30 (30 bit).
		size_t offset_ = 0;

		/// The amount bytes in the memory block.
		/// Max value of this field is 2^30 (30 bit).
		size_t size_ = 0;

	public:
		/// Default constructor.
		MemoryInfo()
		{

		}

		/// Constructor.
		///
		/// Allows for a shift parameter which shifts file to the left
		/// and moves bits from the end of offset into the start of file.
		///
		/// @param file the file number.
		/// @param offset the offset into the file.
		/// @param amount the number of bytes.
		MemoryInfo(uint8_t file, uint32_t offset, uint32_t length)
			: size_(length)
		{
			std::bitset<sizeof(uint8_t) * CHAR_BIT> fileBits(file);
			std::bitset<sizeof(uint32_t) * CHAR_BIT> offsetBits(offset);

			fileBits <<= 2;

			fileBits[0] = offsetBits[30];
			fileBits[1] = offsetBits[31];

			offsetBits[30] = false;
			offsetBits[31] = false;

			this->file_ = fileBits.to_ulong();
			this->offset_ = offsetBits.to_ulong();
		}

		/// Gets the file number containing the block.
		/// @return the file number.
		int file() const
		{
			return file_;
		}

		/// Gets the offset where the writeable area start.
		/// @return the offset in the file given in bytes.
		size_t offset() const
		{
			return offset_;
		}

		/// Gets the size of the block.
		/// @return the amount of bytes in the block.
		size_t size() const
		{
			return size_;
		}
	};
}