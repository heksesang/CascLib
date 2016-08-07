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

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include "Casc/Common.hpp"
#include "Casc/Exceptions.hpp"

const char* usageText =
"Usage: casc <location> <mode> <key> [<output_name>]\n\n"
"<location>     - path to the game directory\n"
"<mode>         - valid values: key, hash, filename\n"
"<key>          - the key, hash or filename (depending on the mode) for the file\n"
"<output_name>  - output name of the file";

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cout << usageText << std::endl;
        return 0;
    }

    try
    {
        auto container = std::make_unique<Casc::Container>(argv[1], "Data");

        try
        {
            std::shared_ptr<std::istream> file;
            
            if (strcmp(argv[2], "key") == 0)
            {
                file = container->openFileByKey(std::string(argv[3]));
            }
            else if (strcmp(argv[2], "hash") == 0)
            {
                file = container->openFileByHash(std::string(argv[3]));
            }
            else if (strcmp(argv[2], "filename") == 0)
            {
                return -1;
            }
            else
            {
                std::cout << usageText << std::endl;
                return 0;
            }
            
            std::fstream fs;

            try
            {
                if (argc > 4)
                {
                    fs.open(argv[4], std::ios_base::out | std::ios_base::binary);
                }
                else
                {
                    fs.open(argv[3], std::ios_base::out | std::ios_base::binary);
                }
            }
            catch (...)
            {
                std::cout << "Failed to open output file for writing." << std::endl;
                return -1;
            }

            file->seekg(0, std::ios_base::end);
            auto size = file->tellg();

            if (size <= 0)
            {
                std::cout << "Invalid file size." << std::endl;
                return -1;
            }

            file->seekg(0, std::ios_base::beg);
            
            try
            {
                std::unique_ptr<char[]> arr = std::make_unique<char[]>((size_t)size);
                file->read(arr.get(), (size_t)size);

                try
                {
                    fs.write(arr.get(), size);
                    fs.close();
                }
                catch (...)
                {
                    std::cout << "Failed to write the file data." << std::endl;
                    return -1;
                }
            }
            catch (...)
            {
                std::cout << "Failed to read the file data." << std::endl;
                return -1;
            }
        }
        catch (Casc::Exceptions::KeyDoesNotExistException &ex)
        {
            std::stringstream ss;
            
            ss << "Couldn't find a file with the given key (" << ex.key << ").";

            std::cout << ss.str() << std::endl;
            return -1;
        }
        catch (Casc::Exceptions::HashDoesNotExistException &ex)
        {
            std::stringstream ss;

            ss << "Couldn't find a file with the given key (" << ex.hash << ").";

            std::cout << ss.str() << std::endl;
            return -1;
        }
        catch (Casc::Exceptions::FilenameDoesNotExistException &ex)
        {
            std::stringstream ss;

            ss << "Couldn't find a file with the given filename (" << ex.filename << ").";

            std::cout << ss.str() << std::endl;
            return -1;
        }
    }
    catch (Casc::Exceptions::CascException &ex)
    {
        std::stringstream ss;

        ss << "Failed to open the CASC container (" << ex.what() << ").";

        std::cout << ss.str() << std::endl;
        return -1;
    }
    
    return 0;
}
