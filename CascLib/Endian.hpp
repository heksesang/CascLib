#pragma once

#include <bitset>

namespace Casc
{
	namespace Endian
	{
		template <typename T>
		inline T readLE(char* arr)
		{
			return readLE<T>(reinterpret_cast<unsigned char*>(arr));
		}

		template <typename T>
		inline T readLE(unsigned char* arr)
		{
			T output{};

			for (int i = 0; i < sizeof(T); ++i)
			{
				output |= arr[i] << i * 8;
			}

			return output;
		}

		template <typename T>
		inline T readLE(T value)
		{
			return readLE<T>(reinterpret_cast<unsigned char*>(&value));
		}

		template <typename T>
		inline T readBE(char* arr)
		{
			return readBE<T>(reinterpret_cast<unsigned char*>(arr));
		}

		template <typename T>
		inline T readBE(unsigned char* arr)
		{
			T output{};

			for (int i = sizeof(T) - 1; i >= 0; --i)
			{
				output |= arr[i] << ((sizeof(T) - 1) - i) * 8;
			}

			return output;
		}

		template <typename T>
		inline T readBE(T value)
		{
			return readBE<T>(reinterpret_cast<unsigned char*>(&value));
		}
	}
}