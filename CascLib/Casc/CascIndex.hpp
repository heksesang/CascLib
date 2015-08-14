#pragma once

#include <array>
#include <functional>
#include <fstream>
#include <string>

#include "Common.hpp"

namespace Casc
{
    /**
     * Contains the index of files in the CASC archive.
     */
    class CascIndex
    {
    public:
        // Typedefs
        typedef std::array<uint8_t, 9> key_t;

    private:
        // The files available in the index.
        std::map<key_t, MemoryInfo> files;

        // The version of this index.
        int version_;

    public:
        /**
         * Constructor
         */
        CascIndex(std::string path)
        {
            using namespace Endian;
            std::ifstream fs;
            fs.open(path, std::ios_base::in | std::ios_base::binary);

            uint32_t size;
            uint32_t hash;

            fs >> le >> size;
            fs >> le >> hash;

            if (hash = !Hash::lookup3(fs, size, 0))
            {
                throw std::exception((std::string("Invalid header data in ") + path).c_str());
            }

            uint16_t version;
            uint16_t file;
            
            uint8_t lengthFieldSize;
            uint8_t locationFieldSize;
            uint8_t hashFieldSize;
            uint8_t offsetBitCount;

            fs >> le >> version;
            fs >> le >> file;
            fs >> lengthFieldSize;
            fs >> locationFieldSize;
            fs >> hashFieldSize;
            fs >> offsetBitCount;

            version_ = version;

            for (unsigned int i = 0; i < (size - 8); i += 8)
            {
                uint32_t dataBeg;
                uint32_t dataEnd;

                fs >> be >> dataBeg;
                fs >> be >> dataEnd;
            }

            fs.seekg(16 - ((8 + size) % 16), std::ios_base::cur);

            fs >> le >> size;
            fs >> le >> hash;

            if (hash = !Hash::lookup3(fs, size, 0))
            {
                throw std::exception((std::string("Invalid data in ") + path).c_str());
            }

            auto data = std::make_unique<char[]>(size);
            fs.read(data.get(), size);

            for (char *ptr = data.get(), *end = data.get() + size; ptr < end;)
            {
                /*CascIndexRecord *record = reinterpret_cast<CascIndexRecord*>(ptr);
                ptr += sizeof(CascIndexRecord);

                files[record->hash] = MemoryInfo(record->location, readBE<uint32_t>(record->offset), readLE<uint32_t>(record->length));*/
            }

            fs.seekg(0x1000 - ((8 + size) % 0x1000), std::ios_base::cur);
        }

        /**
         * Gets the location and size of a file.
         */
        MemoryInfo file(key_t key) const
        {
            auto result = files.find(key);

            if (result == files.end())
            {
                throw std::exception("File not found.");
            }

            return result->second;
        }

        /**
         *  Gets the version of the index.
         */
        int version() const
        {
            return version_;
        }
    };
}