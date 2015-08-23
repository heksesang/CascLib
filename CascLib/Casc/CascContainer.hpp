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

#ifdef _MSC_VER
#include <experimental/filesystem>
#else
#include <boost/filesystem.hpp>
#endif
#include <iomanip>
#include <locale>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "Common.hpp"
#include "md5.hpp"

#include "CascShmem.hpp"
#include "CascBlteHandler.hpp"
#include "CascBuildInfo.hpp"
#include "CascConfiguration.hpp"
#include "CascEncoding.hpp"
#include "CascIndex.hpp"
#include "CascStream.hpp"
#include "CascRootHandler.hpp"
#include "CascLayoutDescriptor.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * A container for a CASC archive.
     */
    class CascContainer
    {
    private:
        typedef std::pair<CascChunkDescriptor, std::vector<char>> descriptor_type;
        typedef std::wstring_convert<deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>> conv_type;
        
        template <typename T, size_t Size>
        size_t copyToVector(const std::array<T, Size> &&src, std::vector<T> &dest, size_t offset) const
        {
            std::copy(src.begin(), src.end(), dest.begin() + offset);
            return offset + Size;
        }

        template <typename T, size_t Size>
        size_t copyToVector(const std::array<T, Size> &src, std::vector<T> &dest, size_t offset) const
        {
            std::copy(src.begin(), src.end(), dest.begin() + offset);
            return offset + Size;
        }

        std::vector<char> createDataHeader(const std::vector<char> &blteHeader, const std::vector<descriptor_type> &chunks) const
        {
            auto blteHeaderSize = blteHeader.size();
            auto dataSize = DataHeaderSize + blteHeader.size() +
                std::accumulate(chunks.begin(), chunks.end(), 0,
                    [](uint32_t value, descriptor_type descriptor) { return value + descriptor.second.size(); });

            std::vector<char> header(DataHeaderSize, '\0');

            auto hash = Hex<16, char>(md5(blteHeader)).data();

            copyToVector(hash, header, 0);
            copyToVector(writeLE<uint32_t>(dataSize), header, 16);

            return std::move(header);
        }

        std::vector<char> createBlteHeader(const std::vector<descriptor_type> &chunks) const
        {
            auto dataSize = std::accumulate(chunks.begin(), chunks.end(), 0,
                [](uint32_t value, descriptor_type descriptor) { return value + descriptor.second.size(); });
            
            auto headerSize = 8 + 8 + 24 * chunks.size();

            size_t pos = 0;

            std::vector<char> header(headerSize, '\0');

            *reinterpret_cast<uint32_t*>(&header[pos]) = BlteSignature;
            pos += sizeof(uint32_t);

            pos = copyToVector(writeBE<uint32_t>(headerSize), header, pos);

            pos = copyToVector(writeBE<uint16_t>(0xF00), header, pos);

            pos = copyToVector(writeBE<uint16_t>(chunks.size()), header, pos);

            for (auto &chunk : chunks)
            {
                pos = copyToVector(writeBE<uint32_t>(chunk.second.size()), header, pos);
                pos = copyToVector(writeBE<uint32_t>(chunk.first.count()), header, pos);
                
                auto hash = Hex<16, char>(md5(chunk.second)).data();

                pos = copyToVector(hash, header, pos);
            }

            return std::move(header);
        }

        std::vector<descriptor_type> createChunkData(std::istream &stream, const CascLayoutDescriptor &descriptor) const
        {
            std::vector<descriptor_type> chunks;

            for (auto &chunk : descriptor.chunks())
            {
                stream.seekg(chunk.begin(), std::ios_base::beg);
                chunks.emplace_back(chunk,
                    blteHandlers.at(chunk.mode())->write(stream, chunk.count()));
            }

            return std::move(chunks);
        }

        std::vector<char> createData(std::istream &stream, const CascLayoutDescriptor &descriptor) const
        {
            auto chunks = createChunkData(stream, descriptor);
            auto blteHeader = createBlteHeader(chunks);
            auto dataHeader = createDataHeader(blteHeader, chunks);

            auto dataSize = dataHeader.size() + blteHeader.size() +
                std::accumulate(chunks.begin(), chunks.end(), 0,
                    [](uint32_t value, descriptor_type descriptor) { return value + descriptor.second.size(); });

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
        std::shared_ptr<CascStream<false>> openFileByKey(const std::string &key) const
        {
            return openStream<false>(findFileLocation(key));
        }

        std::shared_ptr<CascStream<false>> openFileByHash(const std::string &hash) const
        {
            return openFileByKey(encoding->findKey(hash));
        }

        std::shared_ptr<CascStream<false>> openFileByName(const std::string &name) const
        {
            auto root = openFileByHash(this->buildConfig()["root"].front());

            std::array<char, 4> magic;
            root->read(&magic[0], 4);
            root->seekg(0, std::ios_base::beg);

            return openFileByKey(encoding->findKey(rootHandlers.at(magic)->findHash(name)));
        }

        MemoryInfo write(std::istream &stream, CascLayoutDescriptor &descriptor)
        {
            auto arr = createData(stream, descriptor);
            auto loc = shmem_.reserveSpace(arr.size());
            auto out = openStream<true>(loc);

            out->write(arr.data(), arr.size());
            out->close();

            shmem_.writeFile();

            return loc;
        }

        template <typename T>
        void registerHandler()
        {
            CascBlteHandler* handler = new T;
            this->blteHandlers[handler->compressionMode()] = std::shared_ptr<CascBlteHandler>(handler);
        }

        void registerHandlers(std::vector<std::shared_ptr<CascBlteHandler>> handlers)
        {
            for (std::shared_ptr<CascBlteHandler> handler : handlers)
                this->blteHandlers[handler->compressionMode()] = handler;
        }

        void registerHandlers(std::vector<std::shared_ptr<CascRootHandler>> handlers)
        {
            for (std::shared_ptr<CascRootHandler> handler : handlers)
                this->rootHandlers[handler->fileMagic()] = handler;
        }

    private:
        static const int BlteSignature = 0x45544C42;
        static const int DataHeaderSize = 30U;

        // The path of the archive folder.
        std::string path_;

        // The build info.
        CascBuildInfo buildInfo_;

        // The build configuration.
        CascConfiguration buildConfig_;

        // The CDN configuration.
        CascConfiguration cdnConfig_;

        // The SHMEM.
        CascShmem shmem_;

        // The file indices.
        std::vector<CascIndex> indices_;

        // The encoding file.
        std::unique_ptr<CascEncoding> encoding;

        // Chunk handlers
        std::map<char, std::shared_ptr<CascBlteHandler>> blteHandlers;

        // Chunk handlers
        std::map<std::array<char, 4>, std::shared_ptr<CascRootHandler>> rootHandlers;

        /**
         * Finds the location of a file.
         *
         * @param key   the key of the file.
         */
        MemoryInfo findFileLocation(const std::string &key) const
        {
            auto bytes = Hex<9>(key).data();

            for (auto index : indices_)
            {
                try
                {
                    return index.file(bytes);
                }
                catch (FileNotFoundException&) // TODO: Better exception handling
                {
                    continue;
                }
            }

            throw FileNotFoundException(key);
        }

        /**
         * Opens a stream at a given location.
         *
         * @param loc	the location of the data to stream.
         * @return		a stream object.
         */
        template <bool Writeable>
        std::shared_ptr<CascStream<Writeable>> openStream(MemoryInfo loc) const
        {
            std::stringstream ss;
            conv_type conv;

            ss << shmem_.path() << conv.to_bytes(std::wstring{ fs::path::preferred_separator }) << "data." << std::setw(3) << std::setfill('0') << loc.file();
            
            return std::make_shared<CascStream<Writeable>>(
                ss.str(), loc.offset(),
                mapToVector(this->blteHandlers));
        }

    public:
        /**
         * Default constructor.
         */
        CascContainer()
        {
            registerHandler<DefaultHandler>();
        }

        /**
         * Constructor.
         *
         * @param path	the path of the archive folder.
         */
        CascContainer(std::string path,
            std::vector<std::shared_ptr<CascBlteHandler>> blteHandlers = {},
            std::vector<std::shared_ptr<CascRootHandler>> rootHandlers = {})
            : CascContainer()
        {
            registerHandlers(blteHandlers);
            registerHandlers(rootHandlers);
            load(path);
        }

        /**
         * Move constructor.
         */
        CascContainer(CascContainer &&) = default;

        /**
         * Move operator.
         */
        CascContainer &operator= (CascContainer &&) = default;

        /**
         * Destructor.
         */
        virtual ~CascContainer()
        {
        }

        /**
         * Loads a CASC archive into this container.
         *
         * @param path	the path of the archive folder.
         */
        void load(std::string path)
        {
            path_ = path;
            buildInfo_.parse(path + ".build.info");

            FileSearch fs({
                buildInfo_.build(0).at("Build Key"),
                buildInfo_.build(0).at("CDN Key"),
                "shmem"
            }, path_);

            buildConfig_.parse(fs.results().at(0));
            cdnConfig_.parse(fs.results().at(1));
            shmem_.parse(fs.results().at(2), path);

            for (size_t i = 0; i < shmem_.versions().size(); ++i)
            {
                std::stringstream ss;
                //conv_type conv;ss << shmem_.path() << conv.to_bytes({ fs::path::preferred_separator });

                
                ss << std::setw(2) << std::setfill('0') << std::hex << i;
                ss << std::setw(8) << std::setfill('0') << std::hex << shmem_.versions().at(i);
                ss << ".idx";

                indices_.push_back(ss.str());
            }

            encoding = std::make_unique<CascEncoding>(openFileByKey(buildConfig_["encoding"].back()));
        }

        const std::string &path() const
        {
            return path_;
        }

        const CascBuildInfo &buildInfo() const
        {
            return buildInfo_;
        }

        const CascConfiguration &buildConfig() const
        {
            return buildConfig_;
        }

        const CascConfiguration &cdnConfig() const
        {
            return cdnConfig_;
        }

        const CascShmem &shmem() const
        {
            return shmem_;
        }
    };
}