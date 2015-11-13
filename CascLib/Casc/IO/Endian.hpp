/*
* Copyright 2015 Gunnar Lilleaasen
*
* This file is part of CascLib.
*
* CascLib is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* CascLib is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CascLib.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../Exceptions.hpp"

#include "../lookup3.hpp"
#include "../md5.hpp"

#include "EndianType.hpp"

namespace Casc
{
    namespace IO
    {
        namespace Endian
        {
            template <IO::EndianType Type, typename T, typename InputIt>
            inline T read(InputIt first, InputIt last)
            {
                using namespace Casc::Exceptions;

                T output{};
                auto it = first;

                typedef const typename std::make_unsigned<typename std::iterator_traits<InputIt>::value_type>::type* unsigned_ptr;
                typedef const typename std::make_signed<typename std::iterator_traits<InputIt>::value_type>::type* signed_ptr;

                switch (Type)
                {
                case IO::EndianType::Little:
                    for (it = first; it != last; ++it)
                    {
                        if (std::is_unsigned<T>::value)
                        {
                            output |= *reinterpret_cast<unsigned_ptr>(&*it) << (it - first) * 8;
                        }
                        else if (std::is_signed<T>::value)
                        {
                            output |= *reinterpret_cast<signed_ptr>(&*it) << (it - first) * 8;
                        }
                    }
                    break;

                case IO::EndianType::Big:
                    for (it = last - 1; it >= first; --it)
                    {
                        if (std::is_unsigned<T>::value)
                        {
                            output |= *reinterpret_cast<unsigned_ptr>(&*it) << ((sizeof(T) - 1) - (it - first)) * 8;
                        }
                        else if (std::is_signed<T>::value)
                        {
                            output |= *reinterpret_cast<signed_ptr>(&*it) << ((sizeof(T) - 1) - (it - first)) * 8;
                        }
                    }
                    break;
                }

                return output;
            }

            template <IO::EndianType Type, typename T, typename InputIt>
            inline T read(InputIt first)
            {
                auto last = first + sizeof(T);
                return read<Type, T>(first, last);
            }

            template <IO::EndianType Type, typename T, bool Increment, typename InputIt>
            inline T read(InputIt &first)
            {
                auto last = first + sizeof(T);
                auto val = read<Type, T>(first, last);
                if (Increment)
                    first += sizeof(T);
                return val;
            }

            template <IO::EndianType Type, typename T>
            inline std::array<char, sizeof(T)> write(T value)
            {
                std::array<char, sizeof(T)> output{};
                int i;

                switch (Type)
                {
                case IO::EndianType::Little:
                    for (i = 0; i < sizeof(T); ++i)
                    {
                        output[i] = (value >> i * 8) & 0xFF;
                    }
                    break;

                case IO::EndianType::Big:
                    for (i = 0; i < sizeof(T); ++i)
                    {
                        output[(sizeof(T) - 1) - i] = (value >> i * 8) & 0xFF;
                    }
                    break;
                }

                return output;
            }
        }
    }
}