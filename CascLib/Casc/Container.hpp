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

#ifdef _MSC_VER
#include <experimental/filesystem>
#else
#include <experimental/filesystem>
#endif
#include <fstream>
#include <iomanip>
#include <locale>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Common.hpp"
#include "Exceptions.hpp"

#include "md5.hpp"

#include "Filesystem/Root.hpp"
#include "IO/Handler.hpp"
#include "IO/Stream.hpp"
#include "IO/StreamAllocator.hpp"
#include "Parsers/Text/BuildInfo.hpp"
#include "Parsers/Text/Configuration.hpp"
#include "Parsers/Text/EncodingBlock.hpp"
#include "Parsers/Binary/Encoding.hpp"
#include "Parsers/Binary/Index.hpp"
#include "Parsers/Binary/ShadowMemory.hpp"
#include "Parsers/Binary/Reference.hpp"

namespace Casc
{
    /**
     * A container for a CASC archive.
     */
    class Container
    {
    private:
        typedef std::pair<Parsers::Text::EncodingBlock, std::vector<char>> descriptor_type;

    public:
        std::shared_ptr<IO::Stream> openFileByKey(Hex key) const
        {
            return allocator->data(findFileLocation(key));
        }

        std::shared_ptr<IO::Stream> openFileByHash(Hex hash) const
        {
            auto fi = encoding->findFileInfo(hash);
            auto enc = encoding->findEncodedFileInfo(fi.keys.at(0));
            return openFileByKey(enc.key);
        }

        std::shared_ptr<IO::Stream> openFileByName(std::string path) const
        {
            auto hash = root->find(path);
            return openFileByHash(hash);
        }

    private:
        static const int BlteSignature = 0x45544C42;
        static const int DataHeaderSize = 30U;

        // The path of the game directory.
        std::string path;

        // The relative path of the data directory.
        std::string dataPath;

        // The stream allocator.
        std::shared_ptr<IO::StreamAllocator> allocator;

        // The build info.
        Parsers::Text::BuildInfo buildInfo;

        // The build configuration.
        Parsers::Text::Configuration buildConfig;

        // The CDN configuration.
        Parsers::Text::Configuration cdnConfig;

        // The shadow memory.
        Parsers::Binary::ShadowMemory shadowMemory;

        // The file indices.
        std::shared_ptr<Parsers::Binary::Index> index;

        // The encoding file.
        std::shared_ptr<Parsers::Binary::Encoding> encoding;

        // Filesystem root.
        std::shared_ptr<Filesystem::Root> root;

        /**
         * Finds the location of a file.
         */
        Parsers::Binary::Reference findFileLocation(Hex key) const
        {
            return index->find(key.begin(), key.begin() + 9);
        }

    public:
        /**
         * Constructor.
         */
        Container(const std::string path, const std::string dataPath) :
            allocator(new IO::StreamAllocator(path + "\\" + dataPath)),
            buildInfo(path + "\\.build.info"),
            buildConfig(allocator->config<true, false>(buildInfo.build(0).at("Build Key"))),
            cdnConfig(allocator->config<true, false>(buildInfo.build(0).at("CDN Key"))),
            shadowMemory(allocator->shmem<true, false>()),
            index(new Parsers::Binary::Index(shadowMemory.versions(), allocator)),
            encoding(new Parsers::Binary::Encoding(
                index->find(Hex(buildConfig["encoding"].back().substr(0, 18U))), allocator)),
            root(new Filesystem::Root(getProgramCode(buildConfig["build-uid"].front()),
                buildConfig["root"].front(), encoding, index, allocator))
        {
        }

        /**
         * Move constructor.
         */
        Container(Container &&) = default;

        /**
         * Move operator.
         */
        Container &operator= (Container &&) = default;

        /**
         * Destructor.
         */
        virtual ~Container() = default;
    };
}
