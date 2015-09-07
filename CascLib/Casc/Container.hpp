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

        template <typename SrcContainer, typename DestContainer>
        size_t copyToVector(const SrcContainer &src, DestContainer &dest, size_t offset) const
        {
            std::copy(std::begin(src), std::end(src), std::begin(dest) + offset);
            return offset + (std::end(src) - std::begin(src));
        }

        std::vector<char> createDataHeader(const std::vector<char> &blteHeader, const std::vector<descriptor_type> &blocks) const
        {
            auto blteHeaderSize = blteHeader.size();
            auto dataSize = DataHeaderSize + blteHeader.size() +
                std::accumulate(blocks.begin(), blocks.end(), 0,
                    [](uint32_t value, descriptor_type block) { return value + block.second.size(); });

            std::vector<char> header(DataHeaderSize, '\0');

            auto hash = Hex(md5(blteHeader)).data();
            
            copyToVector(hash, header, 0);
            copyToVector(Functions::Endian::write<IO::EndianType::Little, uint32_t>(dataSize), header, 16);

            return std::move(header);
        }

        std::vector<char> createBlteHeader(const std::vector<descriptor_type> &blocks) const
        {
            auto dataSize = std::accumulate(blocks.begin(), blocks.end(), 0,
                [](uint32_t value, descriptor_type block) { return value + block.second.size(); });
            
            auto headerSize = 8 + 8 + 24 * blocks.size();

            size_t pos = 0;

            std::vector<char> header(headerSize, '\0');

            *reinterpret_cast<uint32_t*>(&header[pos]) = BlteSignature;
            pos += sizeof(uint32_t);

            pos = copyToVector(Functions::Endian::write<IO::EndianType::Big, uint32_t>(headerSize), header, pos);
            pos = copyToVector(Functions::Endian::write<IO::EndianType::Big, uint16_t>(0xF00), header, pos);
            pos = copyToVector(Functions::Endian::write<IO::EndianType::Big, uint16_t>((uint16_t)blocks.size()), header, pos);

            for (auto &block : blocks)
            {
                pos = copyToVector(Functions::Endian::write<IO::EndianType::Big, uint32_t>((uint16_t)block.second.size()), header, pos);
                pos = copyToVector(Functions::Endian::write<IO::EndianType::Big, uint32_t>((uint16_t)block.first.size()), header, pos);
                
                auto hash = Hex(md5(block.second)).data();

                pos = copyToVector(hash, header, pos);
            }

            return std::move(header);
        }

        std::vector<descriptor_type> createChunkData(std::istream &stream, const std::vector<Parsers::Text::EncodingBlock> &blocks) const
        {
            std::vector<descriptor_type> chunks;

            /*auto offset = 0U;
            for (auto &block : blocks)
            {
                stream.seekg(offset, std::ios_base::beg);
                chunks.emplace_back(block,
                    handlers.at(block.mode())->encode(stream, block.size()));
                offset += block.size();
            }*/

            return std::move(chunks);
        }

        std::vector<char> createData(std::istream &stream, const std::vector<Parsers::Text::EncodingBlock> &blocks) const
        {
            auto chunks = createChunkData(stream, blocks);
            auto blteHeader = createBlteHeader(chunks);
            auto dataHeader = createDataHeader(blteHeader, chunks);

            auto dataSize = dataHeader.size() + blteHeader.size() +
                std::accumulate(chunks.begin(), chunks.end(), 0,
                    [](uint32_t value, descriptor_type block) { return value + block.second.size(); });

            std::vector<char> data(dataSize, '\0');
            auto iter = data.begin();
            
            std::copy(dataHeader.begin(), dataHeader.end(), iter);
            iter += dataHeader.size();

            std::copy(blteHeader.begin(), blteHeader.end(), iter);
            iter += blteHeader.size();

            for (auto &chunk : chunks)
            {
                std::copy(chunk.second.begin(), chunk.second.end(), iter);
                iter += chunk.second.size();
            }

            return std::move(data);
        }

    public:
        std::shared_ptr<IO::Stream<false>> openFileByKey(const std::string key) const
        {
            return allocator->allocate<false>(findFileLocation(key));
        }

        std::shared_ptr<IO::Stream<false>> openFileByHash(const std::string hash) const
        {
            return openFileByKey(encoding->find(hash).at(0).string());
        }

        Parsers::Binary::Reference write(std::istream &stream, std::vector<Parsers::Text::EncodingBlock> &blocks)
        {
            auto arr = createData(stream, blocks);
            
            auto loc = shadowMemory.reserveSpace(arr.size());
            
            auto out = allocator->allocate<true>(loc);
            out->write(arr.data(), arr.size());
            out->close();

            std::array<char, 16> key;
            std::reverse_copy(arr.begin(), arr.begin() + 16, key.begin());

            loc = Parsers::Binary::Reference(key.begin(), key.begin() + 9, loc.file(), loc.offset(), loc.size());

            index->insert(key.begin(), key.begin() + 9, loc);
            index->write();

            return loc;
        }

    private:
        static const int BlteSignature = 0x45544C42;
        static const int DataHeaderSize = 30U;

        // The path of the game directory.
        std::string path;

        // The relative path of the data directory.
        std::string dataPath;

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

        // The stream allocator.
        std::shared_ptr<IO::StreamAllocator> allocator;

        // The encoding file.
        std::shared_ptr<Parsers::Binary::Encoding> encoding;

        // Filesystem root.
        std::shared_ptr<Filesystem::Root> root;

        /**
         * Finds the location of a file.
         */
        Parsers::Binary::Reference findFileLocation(const Hex key) const
        {
            return index->find(key.begin(), key.begin() + 9);
        }

    public:
        /**
         * Constructor.
         */
        Container(const std::string path, const std::string dataPath) :
            buildInfo(path + ".build.info"),
            buildConfig(Functions::createPath(
                path, dataPath, IO::DataFolders::Config, buildInfo.build(0).at("Build Key"))),
            cdnConfig(Functions::createPath(
                path, dataPath, IO::DataFolders::Config, buildInfo.build(0).at("CDN Key"))),
            shadowMemory(Functions::createPath(
                path, dataPath, IO::DataFolders::Data, "shmem")),
            index(new Parsers::Binary::Index(Functions::createPath(
                path, dataPath, IO::DataFolders::Data, ""), shadowMemory.versions())),
            allocator(new IO::StreamAllocator(
                Functions::createPath(path, dataPath, IO::DataFolders::Data, ""))),
            encoding(new Parsers::Binary::Encoding(buildConfig["encoding"].back(), index, allocator)),
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