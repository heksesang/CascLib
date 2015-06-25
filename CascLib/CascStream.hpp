#pragma once

#include <iostream>
#include "CascBuffer.hpp"

namespace Casc
{
	/**
	 * A stream that uses the CascBuffer as the underlying buffer.
	 * The class provides methods to easily control the underlying buffer.
	 */
	class CascStream : public std::istream
	{
		// The underlying buffer which allows direct data streaming.
		CascBuffer buffer_;

	public:
		/**
		 * Default constructor.
		 */
		CascStream()
			: std::istream(&buffer_)
		{
		}

		/**
		 * Constructor.
		 *
		 * @param filename	the filename of the CASC file.
		 * @param offset	the offset where the file starts.
		 */
		CascStream(const std::string &filename, size_t offset)
			: std::istream(&buffer_)
		{
			open(filename, offset);
		}

		/**
		 * Destructor.
		 */
		~CascStream() override
		{
			if (is_open())
			{
				close();
			}
		}

		/**
		 * Opens a file from the currently opened CASC file.
		 *
		 * @param offset	the offset where the file starts.
		 */
		void open(size_t offset)
		{
			buffer_.open(offset);
		}

		/**
		 * Opens a file in a CASC file.
		 *
		 * @param filename	the filename of the CASC file.
		 * @param offset	the offset where the file starts.
		 */
		void open(const char *filename, size_t offset)
		{
			buffer_.open(filename, offset);
		}

		/**
		 * Opens a file in a CASC file.
		 *
		 * @param filename	the filename of the CASC file.
		 * @param offset	the offset where the file starts.
		 */
		void open(const std::string &filename, size_t offset)
		{
			open(filename.c_str(), offset);
		}

		/**
		 * Closes the currently opened CASC file.
		 */
		void close()
		{
			buffer_.close();
		}

		/**
		 * Checks if the buffer is open.
		 *
		 * @return true if the buffer is open, false if the buffer is closed.
		 */
		bool is_open() const
		{
			return buffer_.is_open();
		}
	};
}