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

#include <fstream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>
#include <unordered_map>

#include "../../Common.hpp"
#include "../../Exceptions.hpp"
#include "../../IO/Endian.hpp"

#include "Parser.hpp"

namespace Casc::Parsers::Binary
{
    struct FileInfo
    {
        Hex hash;
        size_t size;
        std::vector<Hex> keys;
    };

    struct EncodedFileInfo
    {
        Hex key;
        size_t size;
        std::string params;
    };

    struct FileLookupTableData
    {
        std::vector<std::pair<Hex, Hex>> headers;
        std::vector<char> table;
        size_t hashSize;
    };

    /**
     * Maps decoded file hashes to encoded file hashes.
     */
    class Encoding
    {
        auto getTableIterators(FileLookupTableData fileInfoTable, size_t index, Hex checksum) const
        {
            auto begin = fileInfoTable.table.begin() + EntrySize * index;
            auto end = begin + EntrySize;

            verify(begin, end, checksum);

            return std::make_pair(begin, end);
        }

    public:
        /**
         * Find the file info for a file hash.
         */
        FileInfo findFileInfo(Hex hash) const
        {
            for (auto i = 0U; i < fileInfoTable.headers.size(); ++i)
            {
                if (fileInfoTable.headers[i].first <= hash)
                {
                    auto index = fileInfoTable.headers.size() - 1 - i;
                    auto checksum = fileInfoTable.headers[i].second;

                    auto files = parseEntry(index, checksum);

                    for (auto it = files.begin(); it != files.end(); ++it)
                    {
                        if (it->hash == hash)
                        {
                            return *it;
                        }
                    }

                    throw Exceptions::HashDoesNotExistException(hash.string());
                }
            }

            throw Exceptions::HashDoesNotExistException(hash.string());
        }

        /**
         * Find the encoding info for a file key.
         */
        EncodedFileInfo findEncodedFileInfo(Hex key) const
        {
            for (auto i = 0U; i < encodedFileInfoTable.headers.size(); ++i)
            {
                if (encodedFileInfoTable.headers[i].first <= key)
                {
                    auto index = encodedFileInfoTable.headers.size() - 1 - i;
                    auto checksum = encodedFileInfoTable.headers[i].second;

                    auto files = parseEncodedEntry(index, checksum);

                    for (auto it = files.begin(); it != files.end(); ++it)
                    {
                        if (it->key == key)
                        {
                            return *it;
                        }
                    }

                    throw Exceptions::KeyDoesNotExistException(key.string());
                }
            }

            throw Exceptions::KeyDoesNotExistException(key.string());
        }

        /**
         * Get file info for a range of files.
         */
        std::vector<FileInfo> createFileInfoList(size_t offset, size_t count) const
        {
            std::vector<FileInfo> list;

            while (insertFileInfo(list, offset, count) != 0) {

            }

            return list;
        }

        /**
         * Insert FileInfo objects into a list.
         */
        template <template<typename, typename> class Container, class Alloc>
        size_t insertFileInfo(Container<FileInfo, Alloc> &destination, size_t &source, size_t &count) const
        {
            if (destination.size() < count && source < fileInfoTable.headers.size()) {
                auto remaining = count - destination.size();

                auto index = fileInfoTable.headers.size() - 1 - source;
                auto checksum = fileInfoTable.headers[source].second;

                auto newEntries = parseEntry(index, checksum);
                auto n = std::min(remaining, newEntries.size());

                newEntries.insert(destination.end(), newEntries.begin(), newEntries.begin() + n);

                source += n;

                return n;
            }

            return 0;
        }

        /**
         * Get encoding info for a range of files.
         */
        std::vector<EncodedFileInfo> createEncodedFileInfoList(size_t offset, size_t count) const
        {
            std::vector<EncodedFileInfo> list;

            while (insertEncodedFileInfo(list, offset, count) != 0) {

            }

            return list;
        }

        /**
         * Insert EncodedFileInfo objects into a list.
         */
        template <template<typename, typename> class Container, class Alloc>
        size_t insertEncodedFileInfo(Container<EncodedFileInfo, Alloc> &destination, size_t &source, size_t &count) const
        {
            if (destination.size() < count && source < fileInfoTable.headers.size()) {
                auto remaining = count - destination.size();

                auto index = encodedFileInfoTable.headers.size() - 1 - source;
                auto checksum = encodedFileInfoTable.headers[source].second;

                auto newEntries = parseEncodedEntry(index, checksum);
                auto n = std::min(remaining, newEntries.size());

                newEntries.insert(destination.end(), newEntries.begin(), newEntries.begin() + n);

                source += n;

                return n;
            }

            return 0;
        }

    private:
        static const uint16_t Signature = 0x454E;
        static const unsigned int HeaderSize = 22U;
        static const unsigned int EntrySize = 4096U;

        /*struct FileLookupTableData
        {
            std::vector<std::pair<Hex, Hex>> headers;
            std::vector<char> table;
            size_t hashSize;
        };*/

        template <typename Ty>
        friend class Parser;

        FileLookupTableData fileInfoTable;
        FileLookupTableData encodedFileInfoTable;

        std::vector<std::string> profiles;

        template <typename T>
        T const& read(std::shared_ptr<std::istream> stream, T &value) const
        {
            char b[sizeof(T)];
            stream->read(b, sizeof(T));

            return value = IO::Endian::read<IO::EndianType::Big, T>(b);
        }

        /*auto getTableIterators(FileLookupTableData fileInfoTable, size_t index, Hex checksum) const
        {
            auto begin = fileInfoTable.table.begin() + EntrySize * index;
            auto end = begin + EntrySize;

            verify(begin, end, checksum);

            return std::make_pair(begin, end);
        }*/

        template <typename Iterator>
        void verify(Iterator begin, Iterator end, Hex expectedChecksum) const
        {
            Hex actualChecksum(md5(begin, end));

            if (actualChecksum != expectedChecksum)
            {
                throw Exceptions::InvalidHashException<uint32_t>(Crypto::lookup3(expectedChecksum, 0), Crypto::lookup3(actualChecksum, 0), "");
            }
        }

        std::vector<FileInfo> parseEntry(size_t index, Hex checksum) const
        {
            std::vector<FileInfo> files;

            auto iterators = getTableIterators(fileInfoTable, index, checksum);

            for (auto it = iterators.first; it < iterators.second;)
            {
                auto keyCount =
                    IO::Endian::read<IO::EndianType::Little, uint16_t>(it);
                it += sizeof(keyCount);

                if (keyCount == 0)
                    break;

                files.emplace_back(readFileInfo(it, keyCount));
            }

            return files;
        }

        template <typename Iterator>
        FileInfo readFileInfo(Iterator &it, size_t keyCount) const
        {
            auto fileSize =
                IO::Endian::read<IO::EndianType::Big, uint32_t>(it);
            it += sizeof(fileSize);

            auto checksumIt = it;
            it += fileInfoTable.hashSize;

            std::vector<Hex> keys;

            for (auto i = 0U; i < keyCount; ++i)
            {
                keys.emplace_back(it, it + fileInfoTable.hashSize);
                it += fileInfoTable.hashSize;
            }

            return FileInfo{ { checksumIt, checksumIt + fileInfoTable.hashSize }, fileSize, keys };
        }

        std::vector<EncodedFileInfo> parseEncodedEntry(size_t index, Hex checksum) const
        {
            std::vector<EncodedFileInfo> files;

            auto iterators = getTableIterators(encodedFileInfoTable, index, checksum);

            for (auto it = iterators.first; it < iterators.second;)
            {
                files.emplace_back(readEncodedFileInfo(it));
            }

            return files;
        }

        template <typename Iterator>
        EncodedFileInfo readEncodedFileInfo(Iterator &it) const
        {
            auto checksumIt = it;
            it += encodedFileInfoTable.hashSize;

            auto profileIndex =
                IO::Endian::read<IO::EndianType::Big, int32_t>(it);
            it += sizeof(profileIndex);

            ++it;

            auto fileSize =
                IO::Endian::read<IO::EndianType::Big, uint32_t>(it);
            it += sizeof(fileSize);

            auto &profile = profiles[profileIndex];

            return profileIndex >= 0 ? EncodedFileInfo{ { checksumIt, checksumIt + encodedFileInfoTable.hashSize }, fileSize, profile }
                                     : EncodedFileInfo{ { checksumIt, checksumIt + encodedFileInfoTable.hashSize }, fileSize, "" };
        }

        void parse(std::shared_ptr<std::istream> stream)
        {
            verifySignature(stream);

            stream->seekg(1, std::ios_base::cur); // Skip unknown
            readHashSize(stream);
            stream->seekg(4, std::ios_base::cur); // Skip flags
            readTableSize(stream);
            stream->seekg(1, std::ios_base::cur); // Skip unknown

            Parser<std::string> profileParser;
            profiles = profileParser.readMany(stream);

            readFileInfoHeaders(stream);
            readFileInfoTable(stream);
            readEncodedFileInfoHeaders(stream);
            readEncodedFileInfoTable(stream);

            profiles.push_back(profileParser.readOne(stream));
        }

        void verifySignature(std::shared_ptr<std::istream> stream)
        {
            uint16_t signature;
            read(stream, signature);

            if (signature != Signature)
            {
                throw Exceptions::InvalidSignatureException(signature, Signature);
            }
        }

        void readHashSize(std::shared_ptr<std::istream> stream)
        {
            uint8_t hashSize;
            fileInfoTable.hashSize = read(stream, hashSize);
            encodedFileInfoTable.hashSize = read(stream, hashSize);
        }

        void readTableSize(std::shared_ptr<std::istream> stream)
        {
            uint32_t tableSize;
            fileInfoTable.table.resize(EntrySize * read(stream, tableSize));
            encodedFileInfoTable.table.resize(EntrySize * read(stream, tableSize));
        }

        /**
         * Header functions
         */

        void readFileInfoHeaders(std::shared_ptr<std::istream> stream)
        {
            readHeaders(stream, fileInfoTable);
        }

        void readEncodedFileInfoHeaders(std::shared_ptr<std::istream> stream)
        {
            readHeaders(stream, encodedFileInfoTable);
        }

        void readHeaders(std::shared_ptr<std::istream> stream, FileLookupTableData &lookup) const
        {
            for (auto i = 0U; i < lookup.table.size() / EntrySize; ++i)
            {
                std::vector<char> hash(lookup.hashSize);
                std::vector<char> checksum(lookup.hashSize);

                stream->read(hash.data(), lookup.hashSize);
                stream->read(checksum.data(), lookup.hashSize);

                lookup.headers.emplace_back(std::make_pair(hash, checksum));
            }

            std::reverse(lookup.headers.begin(), lookup.headers.end());
        }

        /**
         * Table functions
         */

        void readFileInfoTable(std::shared_ptr<std::istream> stream)
        {
            readTable(stream, fileInfoTable);
        }

        void readEncodedFileInfoTable(std::shared_ptr<std::istream> stream)
        {
            readTable(stream, encodedFileInfoTable);
        }

        void readTable(std::shared_ptr<std::istream> stream, FileLookupTableData &lookup)
        {
            stream->read(lookup.table.data(), lookup.table.size());
        }

    public:
        /**
         * Constructor.
         */
        Encoding(std::shared_ptr<std::istream> stream)
        {
            parse(stream);
        }

        /**
         * Copy constructor.
         */
        Encoding(const Encoding &) = default;

        /**
         * Move constructor.
         */
        Encoding(Encoding &&) = default;

        /**
         * Copy operator.
         */
        Encoding &operator= (const Encoding &) = default;

        /**
         * Move operator.
         */
        Encoding &operator= (Encoding &&) = default;

        /**
         * Destructor.
         */
        virtual ~Encoding() = default;
    };
}
