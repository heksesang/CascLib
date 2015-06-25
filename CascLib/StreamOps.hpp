#pragma once

#include <fstream>
#include "Endian.hpp"

namespace Casc
{
	static int const index = std::ios_base::xalloc();

	std::ifstream &le(std::ifstream &stream)
	{
		stream.iword(index) = 0;

		return stream;
	}

	std::ifstream &be(std::ifstream &stream)
	{
		stream.iword(index) = 1;

		return stream;
	}

	template <typename T>
	std::ifstream &operator>>(std::ifstream  &input, T &value)
	{
		using namespace Endian;
		char b[sizeof(T)];
		input.read(b, sizeof(T));

		value = *reinterpret_cast<T*>(b);

		if (input.iword(index) == 0)
		{
			value = readLE(value);
		}

		if (input.iword(index) == 1)
		{
			value = readBE(value);
		}

		return input;
	}

	std::ifstream &operator>>(std::ifstream  &input, std::ifstream &func(std::ifstream &stream))
	{
		return func(input);
	}
}