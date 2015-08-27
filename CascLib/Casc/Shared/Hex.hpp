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

#include <array>
#include <iomanip>
#include <sstream>

namespace Casc
{
    class Hex
    {
        std::vector<uint8_t> bytes;
        std::string str;

    public:
        template <typename Container>
        Hex(const Container &container)
            : bytes(std::begin(container), std::end(container))
        {
            std::stringstream ss;

            ss << std::hex
                << std::setfill('0');

            for (auto it = std::begin(bytes); it != std::end(bytes); ++it)
            {
                ss << std::setw(2) << (size_t)*it;
            }

            this->str = ss.str();
        }

        template <typename InputIt>
        Hex(InputIt first, InputIt last)
            : bytes(first, last)
        {
            std::stringstream ss;

            ss << std::hex
               << std::setfill('0');

            for (auto it = first; it != last; ++it)
            {
                ss << std::setw(2) << *it;
            }

            this->str = ss.str();
        }

        Hex(const std::string &str)
            : bytes(str.size() / 2), str(str)
        {
            for (unsigned int i = 0; i < bytes.size(); ++i)
            {
                std::stringstream ss;
                ss << str[i * 2] << str[i * 2 + 1];

                int i1;
                ss >> std::hex >> i1;

                bytes[i] = i1;
            }
        }

        const std::vector<uint8_t> &data()
        {
            return bytes;
        }

        const std::string &string()
        {
            return str;
        }
    };
}