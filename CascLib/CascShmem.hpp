#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>
using namespace std::experimental::filesystem;

#include "Shared/Utils.hpp"
#include "Shared/MemoryInfo.hpp"
#include "Shared/Reference.hpp"
#include "StreamOps.hpp"

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * Contains information about which CASC files are available for writing,
     * and which is the current version of the index lists.
     */
    class CascShmem
    {
        /**
        * Different SHMEM block types:
        * Header
        * Free space table
        */
        enum ShmemType
        {
            Header = 4,
            WriteableMemory = 1
        };

        // The base directory of the archive.
        std::string base_;

        // The directory where the data files are stored.
        std::string path_;

        // The list of versions for IDX files. Contains 16 values for WoD beta.
        std::map<unsigned int, unsigned int> versions_;

        // The list of free spaces of memory in the data files. 
        std::vector<MemoryInfo> memory_;

        /**
         * Reads a chunk of type ShmemType::WriteableMemory.
         * 
         * @param file the file stream to read the data from.
         */
        void readWriteableMemory(std::ifstream &file)
        {
            using namespace Endian;
            uint32_t writeableMemoryCount;
            file >> writeableMemoryCount;

            file.seekg(24, std::ios_base::cur);

            std::array<Ref, 1090> first;
            std::array<Ref, 1090> second;

            file.read(reinterpret_cast<char*>(&first[0]), 1090 * (sizeof(uint8_t) + sizeof(uint32_t)));
            file.read(reinterpret_cast<char*>(&second[0]), 1090 * (sizeof(uint8_t) + sizeof(uint32_t)));

            for (unsigned int i = 0; i < writeableMemoryCount; ++i)
            {
                memory_.push_back(MemoryInfo(
                    second[i].hi,
                    readBE(second[i].lo),
                    readBE(first[i].lo)));
            }
        }

        /**
         * Reads a chunk of type ShmemType::Header.
         *
         * @param file the file stream to read the data from.
         * @param base the path of the base directory.
         */
        void readHeader(std::ifstream &file)
        {
            uint32_t headerSize;
            file >> le >> headerSize;

            std::string path(256, '\0');
            file.read(&path[0], 256);

            int pathTypeLength = path.find_first_of(R"(\)");
            if (pathTypeLength != -1)
            {
                std::string pathType = path.substr(0, pathTypeLength);

                if (pathType.compare("Global") == 0)
                {
                    path.assign(path.substr(pathTypeLength + 1));
                }
            }

            path.resize(path.find_first_of('\0'));
            
            if (v1::path(path).is_relative())
            {
                path = v1::path(base_).append(path).string();
            }

            path_ = path;

            for (v1::directory_iterator iter(path), end; iter != end; ++iter)
            {
                if (!v1::is_directory(iter->path()))
                {
                    auto ext = iter->path().extension();
                    auto fn = iter->path().filename();
                    
                    if (ext.compare(".idx") == 0)
                    {
                        std::stringstream ss;
                        ss << std::hex << fn.string().substr(0, 2);

                        unsigned int index;
                        ss >> index;

                        versions_[index] = 0;
                    }
                }
            }

            auto blockCount = (headerSize - 264 - versions_.size() * sizeof(uint32_t)) / (sizeof(uint32_t) * 2);

            std::vector<std::pair<uint32_t, uint32_t>> blocks(blockCount);

            for (unsigned int i = 0; i < blockCount; ++i)
            {
                file >> le >> blocks[i].first;
                file >> le >> blocks[i].second;
            }

            for (unsigned int i = 0; i < versions_.size(); ++i)
            {
                file >> le >> versions_[i];
            }

            for (unsigned int i = 0; i < blockCount; ++i)
            {
                file.seekg(blocks[i].second);

                uint32_t type = 0;
                file >> le >> type;

                switch (type)
                {
                case ShmemType::Header:
                    break;

                case ShmemType::WriteableMemory:
                    readWriteableMemory(file);
                    break;
                }
            }
        }

        /**
         * Reads a SHMEM file.
         *
         * @param path the path of the SHMEM file.
         */
        void readFile(std::string path)
        {
            using namespace Endian;
            std::ifstream file;
            file.open(path, std::ios_base::in | std::ios_base::binary);

            uint32_t type = 0;
            file >> type;
            type = readLE(type);

            switch (type)
            {
            case ShmemType::Header:
                readHeader(file);
                break;

            case ShmemType::WriteableMemory:
                break;
            }

            file.close();
        }

    public:
        /**
         * Default constructor.
         */
        CascShmem()
        {
        }

        /**
         * Constructor.
         *
         * @param path the path of the SHMEM file.
         * @param base the path of the base directory.
         */
        CascShmem(std::string path, std::string base)
        {
            parse(path, base);
        }

        /**
         * Destructor.
         */
        virtual ~CascShmem()
        {
        }

        /**
         * Parses a SHMEM file.
         *
         * @param path the path of the SHMEM file.
         * @param base the path of the base directory.
         */
        void parse(std::string path, std::string base)
        {
            base_ = base;
            readFile(path);
        }

        /**
         * Gets the directory where the data files are stored.
         *
         * @return the directory path.
         */
        const std::string &path() const
        {
            return path_;
        }

        /**
         * Gets the IDX file versions.
         *
         * @return the list of version numbers.
         */
        const std::map<unsigned int, unsigned int> &versions() const
        {
            return versions_;
        }

        /**
         * Gets the list of free memory blocks in CASC files.
         *
         * @return the list of free memory blocks.
         */
        const std::vector<MemoryInfo> &memoryInfo() const
        {
            return memory_;
        }
    };
}