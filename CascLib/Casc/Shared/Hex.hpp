/*
* Copyright 2014 Gunnar Lilleaasen
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

#include <iomanip>
#include <sstream>

namespace Casc
{
    template <int Size>
    class Hex
    {
        std::array<uint8_t, Size> arr;
        std::string str;

    public:
        Hex(const std::string &str)
        {
            for (unsigned int i = 0; i < arr.size(); ++i)
            {
                std::stringstream ss;
                ss << str[i * 2];
                ss << str[i * 2 + 1];

                int i1;
                ss >> std::hex >> i1;

                arr[i] = i1;
            }

            this->str = str;
        }

        Hex(const std::array<uint8_t, Size> &arr)
        {
            std::stringstream ss;

            ss << std::hex
               << std::setfill('0');

            for (unsigned int c : arr)
            {
                ss << std::setw(2)
                   << c;
            }

            this->str = ss.str();
            this->arr = arr;
        }

        const std::array<uint8_t, Size> &data()
        {
            return arr;
        }

        const std::string &string()
        {
            return str;
        }
    };
}