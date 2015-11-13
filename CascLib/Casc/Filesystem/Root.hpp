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

#include "../Common.hpp"
#include "Handler.hpp"
#include "../Parsers/Binary/Encoding.hpp"
#include "../Parsers/Binary/Index.hpp"
#include "../IO/StreamAllocator.hpp"
#include "../IO/Endian.hpp"

namespace Casc
{
    namespace Filesystem
    {
        class Root
        {
            std::unique_ptr<Handler> handler = nullptr;

        public:
            Root(ProgramCode game, Hex hash, std::shared_ptr<Parsers::Binary::Encoding> encoding = nullptr,
                 std::shared_ptr<Parsers::Binary::Index> index = nullptr,
                 std::shared_ptr<IO::StreamAllocator> allocator = nullptr)
            {
                auto fi = encoding->findFileInfo(hash);
                auto enc = encoding->findEncodedFileInfo(fi.keys[0]);
                auto ref = index->find(fi.keys[0].begin(), fi.keys[0].begin() + 9);
                
                auto stream = allocator->data(ref);

                std::vector<char> buf(fi.size);
                stream->read(buf.data(), buf.size());

                switch (game)
                {
                case ProgramCode::wow:
                case ProgramCode::wowt:
                case ProgramCode::wow_beta:
                    handler = std::make_unique<Impl::WoWHandler>(buf);
                    break;

                default:
                    throw Exceptions::FilesystemException("Unsupported root file format");
                }
            }

            Hex find(std::string path) const
            {
                return handler->findHash(path);
            }
        };
    }
}