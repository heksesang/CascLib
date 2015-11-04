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

#include <functional>
#include <memory>
#include <sstream>

#include "../Common.hpp"

#include "../Parsers/Binary/Reference.hpp"
#include "Stream.hpp"

namespace Casc
{
    namespace IO
    {
        class StreamAllocator
        {
        private:
            /**
            * The path of the CASC root folder.
            */
            std::string basePath;

            /**
            * Create path to a file.
            */
            std::string createPath(IO::DataFolders folder, std::string filename) const
            {
                std::stringstream out;
                out << basePath << PathSeparator;

                switch (folder)
                {
                case IO::DataFolders::Config:
                    out << "config";
                    if (!filename.empty())
                    {
                        out << PathSeparator << filename.substr(0, 2)
                            << PathSeparator << filename.substr(2, 2);
                    }
                    break;

                case IO::DataFolders::Data:
                    out << "data";
                    break;

                case IO::DataFolders::Indices:
                    out << "indices";
                    break;

                case IO::DataFolders::Patch:
                    out << "patch";
                    break;
                }

                if (!filename.empty())
                {
                    out << PathSeparator << filename;
                }

                if (!fs::exists(out.str()))
                {
                    throw Exceptions::FileNotFoundException(out.str());
                }

                return out.str();
            }

            /**
            * Create the stream for a path.
            */
            template <bool Writeable, typename TStream>
            std::shared_ptr<TStream> allocate(std::string path) const
            {
                std::ios_base::openmode mode;

                if (Writeable)
                {
                    mode = std::ios_base::out | std::ios_base::in | std::ios_base::binary;
                }
                else
                {
                    mode = std::ios_base::in | std::ios_base::binary;
                }

                return std::make_shared<TStream>(path, mode);
            }

        public:
            /**
            * Constructor.
            */
            StreamAllocator(const std::string basePath)
                : basePath(basePath)
            {

            }

            /**
            * Config
            */
            template <bool Readable, bool Writeable, typename TStream =
                typename std::conditional<Readable && Writeable, std::fstream,
                    typename std::conditional<Writeable, std::ofstream, std::ifstream >::type>::type >
            std::shared_ptr<TStream> config(std::string hash)
            {
                return allocate<Writeable, TStream>(
                    createPath(DataFolders::Config, hash));
            }

            /**
            * Index
            */
            template <bool Readable, bool Writeable, typename TStream =
                typename std::conditional<Readable && Writeable, std::fstream,
                    typename std::conditional<Writeable, std::ofstream, std::ifstream >::type>::type >
            std::shared_ptr<TStream> index(uint32_t bucket, uint32_t version)
            {
                std::stringstream ss;

                ss << std::setw(2) << std::setfill('0') << std::hex << bucket;
                ss << std::setw(8) << std::setfill('0') << std::hex << version;
                ss << ".idx";

                return allocate<Writeable, TStream>(
                    createPath(DataFolders::Data, ss.str()));
            }

            /**
            * Data
            */
            template <bool Readable, bool Writeable, typename TStream =
                typename std::conditional<Readable && Writeable, std::fstream,
                    typename std::conditional<Writeable, std::ofstream, std::ifstream >::type>::type >
            std::shared_ptr<TStream> data(uint32_t number) const
            {
                std::stringstream ss;

                ss << "data." << std::setw(3) << std::setfill('0') << number;

                return allocate<Writeable, TStream>(
                    createPath(DataFolders::Data, ss.str()));
            }

            template <bool Writeable>
            typename std::enable_if<!Writeable, std::shared_ptr<Stream<Writeable>>>::type
                data(const Parsers::Binary::Reference &ref, std::string parameters = "") const
            {
                std::stringstream ss;

                ss << "data." << std::setw(3) << std::setfill('0') << ref.file();

                return std::make_shared<Stream<Writeable>>(
                    createPath(DataFolders::Data, ss.str()), ref.offset(), parameters);
            }

            template <bool Writeable>
            typename std::enable_if<Writeable, std::shared_ptr<Stream<Writeable>>>::type
                data(typename Stream<Writeable>::insert_func inserter) const
            {
                return std::make_shared<Stream<Writeable>>(basePath, inserter);
            }
        };
    }
}