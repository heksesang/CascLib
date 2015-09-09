#pragma once
#include <zconf.h>
#include <zlib.h>
#include <exception>
#include <string>
#include <sstream>

class ZError : public std::runtime_error
{
public:
	ZError(std::string msg) : std::runtime_error(msg)
	{

	}
};

const size_t ChunkSize = 16384;

/*!
* @brief Base class for Zlib streams
*/
class ZStreamBase
{
public:
	/*!
	* @brief Buffert type
	*/
	typedef unsigned char char_t;

	/*!
	* @brief Constructor
	*/
	ZStreamBase()
	{

	}

	/*!
	* @brief Destructor
	*/
	virtual ~ZStreamBase()
	{

	}

	/*!
	* @brief Byte available in input
	*
	* @return true if not data are available in input
	*/
	bool isInEmpty() const { return z_stream_.avail_in == 0; };

	/*!
	* @briet Byte available in output
	*
	* @return true if not data are available in output
	*/
	bool isOutEmpty() const { return z_stream_.avail_out == 0; };

	/*!
	* @brief Stream input complete on inflate
	*
	* @return true if the stream input is compltete on inflate
	*/
	bool isStreamEnd() const { return stream_end_; }

	/*!
	* @brief Write a buffer to the ZLib stream
	*
	* @param buf pointer to the buffer
	* @param buf_size buffer size
	*/
	void write(char_t* buf, size_t buf_size)
	{
		z_stream_.avail_in = buf_size;
		z_stream_.next_in = reinterpret_cast< Bytef* >(buf);
	}

	/*!
	* @brief Read a buffer from the ZLib stream
	*
	* @param buf pointer to the buffer
	* @param buf_size buffer size
	*/
	virtual void read(char_t* buf, size_t buf_size) = 0;

	/*!
	* @brief Read all ZLib output stream
	*
	* Use the read function for read each data chunk until
	* the output buffer is empty, isOutEmpty return true.
	*
	* @param buf return a new allocated buffer
	* @param buf_size return the size of the allocated buffer
	*/
	void readAll(char_t** buf, size_t& buf_size)
	{
		std::stringstream out_stream;

		char_t out_buf[ChunkSize];

		do
		{
			read(out_buf, ChunkSize);

			size_t out_size = ChunkSize - z_stream_.avail_out;
			out_stream.write(reinterpret_cast< const char* >(out_buf), out_size);

			if (out_stream.bad())
				throw ZError("Error writing temporary stream");

		} while (isOutEmpty());

		buf_size = out_stream.str().size();
		*buf = new char_t[buf_size];
		out_stream.read(reinterpret_cast< std::stringstream::char_type* >(*buf), buf_size);
	}

	/*!
	* @brief Mark as complete the write operation
	*/
	void flush() { flush_ = true; };

protected:
	z_stream z_stream_;
	bool flush_;
	bool stream_end_;
};

class ZDeflateStream : public ZStreamBase
{
	int compression_level_;

public:
	ZDeflateStream(int compression_level/*=Z_DEFAULT_COMPRESSION*/) :
		compression_level_(compression_level)
	{
        z_stream_.zalloc = Z_NULL;
        z_stream_.zfree = Z_NULL;
        z_stream_.opaque = Z_NULL;

		int ret = deflateInit(&z_stream_, compression_level_);

		if (ret != Z_OK)
			throw ZError("Can not initialize deflate stream");
	}

	~ZDeflateStream()
	{
		deflateEnd(&z_stream_);
	}

	void read(char_t* buf, size_t buf_size)
	{
		z_stream_.avail_out = buf_size;
		z_stream_.next_out = reinterpret_cast< Bytef* >(buf);

		int ret = deflate(&z_stream_, flush_ ? Z_FINISH : Z_NO_FLUSH);

		if (ret == Z_STREAM_ERROR)
			throw ZError("Error on deflate");
	}
};

class ZInflateStream : public ZStreamBase
{
	char_t* in_;
	size_t avail_in_;

public:
	ZInflateStream(char_t* in, size_t avail_in)
	{
		z_stream_.zalloc = Z_NULL;
		z_stream_.zfree = Z_NULL;
		z_stream_.opaque = Z_NULL;
		z_stream_.avail_in = avail_in;
		z_stream_.next_in = in;

		int ret = inflateInit(&z_stream_);

		if (ret != Z_OK)
			throw ZError("Can not initialize inflate stream");
	}

	~ZInflateStream()
	{
		inflateEnd(&z_stream_);
	}

	void read(char_t* buf, size_t buf_size)
	{
		z_stream_.avail_out = buf_size;
		z_stream_.next_out = reinterpret_cast< Bytef* >(buf);

		int ret = inflate(&z_stream_, Z_NO_FLUSH);

		switch (ret)
		{
		case Z_STREAM_ERROR:
			throw ZError("The stream structure was inconsistent");
			break;
		case Z_NEED_DICT:
			throw ZError("A preset dictionary is needed");
			break;
		case Z_DATA_ERROR:
			throw ZError("Input data was corrupted");
			break;
		case Z_MEM_ERROR:
			throw ZError("Not enough memory");
			break;
		case Z_STREAM_END:
			stream_end_ = true;
			break;
		}
	}
};