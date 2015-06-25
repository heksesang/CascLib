#pragma once

#include <iomanip>
#include <sstream>

namespace Casc
{
	namespace Hex
	{
		template <int Size>
		inline std::array<char, Size> fromString(std::string str)
		{
			std::array<char, Size> arr;

			for (unsigned int i = 0; i < arr.size(); ++i)
			{
				std::stringstream ss;
				ss << str[i * 2];
				ss << str[i * 2 + 1];

				int i1;
				ss >> std::hex >> i1;

				arr[i] = i1;
			}

			return arr;
		}
	}
}