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
#include <boost/filesystem.hpp>
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
    using namespace Casc::Functions;
    using namespace Casc::Functions::Endian;
    using namespace Casc::Functions::Hash;

    /**
     * A container for a CASC archive.
     */
    class Container
    {
    private:
        typedef std::pair<Parsers::Text::EncodingBlock, std::vector<char>> descriptor_type;

    public:
        std::shared_ptr<IO::Stream<false>> openFileByKey(Hex key, std::string params) const
        {
            return allocator->data<false>(findFileLocation(key), params);
        }

        std::shared_ptr<IO::Stream<false>> openFileByHash(std::string hash) const
        {
            auto key = encoding->findFileInfo(hash).keys.at(0);
            auto enc = encoding->findEncodedFileInfo(key);
            return openFileByKey(enc.key, enc.params);
        }

        std::shared_ptr<IO::Stream<true>> write()
        {
            IO::Stream<true>::insert_func inserter =
                std::bind(&Container::insertFile,
                    this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

            return allocator->data<true>(inserter);
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

        void insertEncoding(Parsers::Binary::Reference ref)
        {
            if (index != nullptr)
            {
                index->insert(ref.key(), ref);
            }
        }

        void insertFile(Parsers::Binary::Reference ref, Hex hash, size_t size)
        {
            if (index != nullptr)
            {
                index->insert(ref.key(), ref);
            }

            if (encoding != nullptr)
            {
                //encoding->insert(hash, ref.key(), size);

                /*IO::Stream<true>::insert_func inserter =
                    std::bind(&Container::insertEncoding,
                        this, std::placeholders::_1);

                auto stream = allocator->allocate<true>(inserter);
                encoding->write(stream);*/
            }
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
            shadowMemory(Functions::createPath(
                path, dataPath, IO::DataFolders::Data, "shmem")),
            index(new Parsers::Binary::Index(shadowMemory.versions(), allocator)),
            encoding(new Parsers::Binary::Encoding(
                index->find(Hex(buildConfig["encoding"].back().substr(0, 18U))), allocator)),
            root(new Filesystem::Root(buildConfig["root"].front(), encoding, index, allocator))
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