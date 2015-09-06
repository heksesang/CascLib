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

#include "Filesystem/RootHandler.hpp"
#include "IO/Handler.hpp"
#include "IO/Stream.hpp"
#include "Parsers/Text/BuildInfo.hpp"
#include "Parsers/Text/Configuration.hpp"
#include "Parsers/Text/EncodingBlock.hpp"
#include "Parsers/Binary/Encoding.hpp"
#include "Parsers/Binary/Index.hpp"
#include "Parsers/Binary/ShadowMemory.hpp"
#include "Parsers/Binary/Reference.hpp"

namespace Casc
{
    using namespace Casc::Shared;
    using namespace Casc::Shared::Functions;
    using namespace Casc::Shared::Functions::Endian;
    using namespace Casc::Shared::Functions::Hash;

    /**
     * A container for a CASC archive.
     */
    class Container
    {
    private:
        typedef std::pair<Parsers::Text::EncodingBlock, std::vector<char>> descriptor_type;
        typedef std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>> conv_type;

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

            auto offset = 0U;
            for (auto &block : blocks)
            {
                stream.seekg(offset, std::ios_base::beg);
                chunks.emplace_back(block,
                    blteHandlers.at(block.mode())->write(stream, block.size()));
                offset += block.size();
            }

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
            return openStream<false>(findFileLocation(key));
        }

        std::shared_ptr<IO::Stream<false>> openFileByHash(const std::string hash) const
        {
            return openFileByKey(encoding_->find(hash).at(0).string());
        }

        std::shared_ptr<IO::Stream<false>> openFileByName(const std::string name) const
        {
            auto root = openFileByHash(this->buildConfig()["root"].front());

            std::array<char, 4> magic;
            root->read(&magic[0], 4);
            root->seekg(0, std::ios_base::beg);

            return openFileByKey(
                encoding_->find(
                    rootHandlers.at(
                        Functions::Endian::read<IO::EndianType::Little, uint32_t>(magic.begin(), magic.end())
                    )->findHash(name)).at(0).string());
        }

        Parsers::Binary::Reference write(std::istream &stream, std::vector<Parsers::Text::EncodingBlock> &blocks)
        {
            auto arr = createData(stream, blocks);
            
            auto loc = shmem_.reserveSpace(arr.size());
            shmem_.writeFile();
            
            auto out = openStream<true>(loc);
            out->write(arr.data(), arr.size());
            out->close();

            std::array<char, 16> key;
            std::reverse_copy(arr.begin(), arr.begin() + 16, key.begin());

            loc = Parsers::Binary::Reference(key.begin(), key.begin() + 9, loc.file(), loc.offset(), loc.size());

            auto bucket = Parsers::Binary::Index::bucket(key.begin(), key.begin() + 9);
            indices_[bucket].insert(key.begin(), key.begin() + 9, loc);

            std::ofstream indexFs;
            indexFs.open(indices_[bucket].path(), std::ios_base::out | std::ios_base::binary);
            indices_[bucket].write(indexFs);
            indexFs.close();

            return loc;
        }

        template <typename T>
        void registerHandler()
        {
            IO::Handler* handler = new T;
            this->blteHandlers[handler->mode()] = std::shared_ptr<IO::Handler>(handler);
        }

        void registerHandlers(std::vector<std::shared_ptr<IO::Handler>> handlers)
        {
            for (std::shared_ptr<IO::Handler> handler : handlers)
                this->blteHandlers[handler->mode()] = handler;
        }

        void registerHandlers(std::vector<std::shared_ptr<Filesystem::RootHandler>> handlers)
        {
            for (std::shared_ptr<Filesystem::RootHandler> handler : handlers)
                this->rootHandlers[handler->signature()] = handler;
        }

    private:
        static const int BlteSignature = 0x45544C42;
        static const int DataHeaderSize = 30U;

        // The path separator for this system.
        const std::string PathSeparator;

        // The path of the game directory.
        std::string path_;

        // The relative path of the data directory.
        std::string dataPath_;

        // The build info.
        Parsers::Text::BuildInfo buildInfo_;

        // The build configuration.
        Parsers::Text::Configuration buildConfig_;

        // The CDN configuration.
        Parsers::Text::Configuration cdnConfig_;

        // The SHMEM.
        Parsers::Binary::ShadowMemory shmem_;

        // The file indices.
        std::vector<Parsers::Binary::Index> indices_;

        // The encoding file.
        std::unique_ptr<Parsers::Binary::Encoding> encoding_;

        // Chunk handlers
        std::map<char, std::shared_ptr<IO::Handler>> blteHandlers;

        // Chunk handlers
        std::map<uint32_t, std::shared_ptr<Filesystem::RootHandler>> rootHandlers;

        /**
         * Finds the location of a file.
         *
         * @param key   the key of the file.
         */
        Parsers::Binary::Reference findFileLocation(const std::string key) const
        {
            auto bytes = Hex(key).data();

            for (auto index : indices_)
            {
                try
                {
                    return index.find(bytes.begin(), bytes.begin() + 9);
                }
                catch (Exceptions::KeyDoesNotExistException&)
                {
                    continue;
                }
            }

            throw Exceptions::KeyDoesNotExistException(key);
        }

        /**
         * Opens a stream at a given location.
         *
         * @param loc	the location of the data to stream.
         * @return		a stream object.
         */
        template <bool Writeable>
        std::shared_ptr<IO::Stream<Writeable>> openStream(Parsers::Binary::Reference loc) const
        {
            std::stringstream ss;
            conv_type conv;

            ss << shmem_.path() << conv.to_bytes(std::wstring{ fs::path::preferred_separator }) << "data." << std::setw(3) << std::setfill('0') << loc.file();

            return std::make_shared<IO::Stream<Writeable>>(
                ss.str(), loc.offset(),
                mapToVector(this->blteHandlers));
        }

    public:
        /**
         * Default constructor.
         */
        Container()
            : PathSeparator(conv_type().to_bytes({ fs::path::preferred_separator }))
        {
            registerHandler<IO::DefaultHandler>();
        }

        /**
         * Constructor.
         *
         * @param path	    the path of the game directory.
         * @param dataPath	the relative path to the data directory.
         */
        Container(const std::string path, const std::string dataPath,
            std::vector<std::shared_ptr<IO::Handler>> blteHandlers = {},
            std::vector<std::shared_ptr<Filesystem::RootHandler>> rootHandlers = {})
            : Container()
        {
            registerHandlers(blteHandlers);
            registerHandlers(rootHandlers);
            load(path,  dataPath);
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
        virtual ~Container()
        {
        }

        /**
         * Loads a CASC archive into this container.
         *
         * @param path	    the path of the game directory.
         * @param dataPath	the relative path to the data directory.
         */
        void load(const std::string path, const std::string dataPath)
        {
            path_ = path;
            dataPath_ = dataPath;
            buildInfo_.parse(path + ".build.info");

            auto buildConfigHash = buildInfo_.build(0).at("Build Key");
            auto cdnConfigHash = buildInfo_.build(0).at("CDN Key");

            std::stringstream buildConfig;
            buildConfig << path_ << PathSeparator << dataPath_
                << PathSeparator << "config"
                << PathSeparator << buildConfigHash.substr(0, 2)
                << PathSeparator << buildConfigHash.substr(2, 2)
                << PathSeparator << buildConfigHash;

            std::stringstream cdnConfig;
            cdnConfig << path_ << PathSeparator << dataPath_
                << PathSeparator << "config"
                << PathSeparator << cdnConfigHash.substr(0, 2)
                << PathSeparator << cdnConfigHash.substr(2, 2)
                << PathSeparator << cdnConfigHash;
            
            std::stringstream shadowMemory;
            shadowMemory << path_ << PathSeparator << dataPath_
                << PathSeparator << "data"
                << PathSeparator << "shmem";

            buildConfig_.parse(buildConfig.str());
            cdnConfig_.parse(cdnConfig.str());
            shmem_.parse(shadowMemory.str(), path);

            for (size_t i = 0; i < shmem_.versions().size(); ++i)
            {
                std::stringstream ss;

                ss << shmem_.path() << PathSeparator;
                ss << std::setw(2) << std::setfill('0') << std::hex << i;
                ss << std::setw(8) << std::setfill('0') << std::hex << shmem_.versions().at(i);
                ss << ".idx";

                if (!fs::exists(ss.str()))
                {
                    throw Exceptions::FileNotFoundException(ss.str());
                }

                indices_.push_back(ss.str());

                /*std::ofstream fs;
                fs.open(ss.str(), std::ios_base::out | std::ios_base::binary);
                indices_.at(i).write(fs);
                fs.close();*/
            }

            encoding_ = std::make_unique<Parsers::Binary::Encoding>(openFileByKey(buildConfig_["encoding"].back()));
        }

        const std::string &path() const
        {
            return path_;
        }

        const Parsers::Text::BuildInfo &buildInfo() const
        {
            return buildInfo_;
        }

        const Parsers::Text::Configuration &buildConfig() const
        {
            return buildConfig_;
        }

        const Parsers::Text::Configuration &cdnConfig() const
        {
            return cdnConfig_;
        }

        const Parsers::Binary::ShadowMemory &shmem() const
        {
            return shmem_;
        }

        const Parsers::Binary::Encoding &encoding() const
        {
            return *encoding_;
        }
    };
}