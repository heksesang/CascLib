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

#include "../lookup3.hpp"

namespace Casc
{
    namespace Crypto
    {
        template <typename Container>
        inline std::pair<uint32_t, uint32_t> lookup3(const Container &container, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
        {
            auto pc = init.first;
            auto pb = init.second;

            hashlittle2(
                &*std::begin(container),
                sizeof(typename Container::value_type) * (std::end(container) - std::begin(container)),
                &pc, &pb);

            return std::make_pair(pc, pb);
        }

        template <typename InputIt>
        inline std::pair<uint32_t, uint32_t> lookup3(InputIt first, InputIt last, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
        {
            auto pc = init.first;
            auto pb = init.second;

            hashlittle2(
                &*first,
                sizeof(typename std::iterator_traits<InputIt>::value_type) * (last - first),
                &pc, &pb);

            return std::make_pair(pc, pb);
        }

        inline std::pair<uint32_t, uint32_t> lookup3(std::ifstream &stream, uint32_t length, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
        {
            auto pc = init.first;
            auto pb = init.second;
            std::vector<char> buffer(length);

            auto pos = stream.tellg();

            stream.read(buffer.data(), length);
            hashlittle2(buffer.data(), length, &pc, &pb);

            stream.seekg(pos);

            return std::make_pair(pc, pb);
        }

        template <typename Container>
        inline uint32_t lookup3(const Container &container, const uint32_t &init)
        {
            return hashlittle(
                &*std::begin(container),
                sizeof(typename Container::value_type) * (std::end(container) - std::begin(container)),
                init);
        }

        template <typename InputIt>
        inline uint32_t lookup3(InputIt first, InputIt last, const uint32_t &init)
        {
            return hashlittle(
                &*first,
                sizeof(typename std::iterator_traits<InputIt>::value_type) * (last - first),
                init);
        }

        inline uint32_t lookup3(std::ifstream &stream, uint32_t length, const uint32_t &init)
        {
            auto hash = init;
            std::vector<char> buffer(length);

            auto pos = stream.tellg();

            stream.read(buffer.data(), length);
            hash = hashlittle(buffer.data(), length, hash);

            stream.seekg(pos);

            return hash;
        }
    }
}