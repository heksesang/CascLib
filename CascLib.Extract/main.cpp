#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../CascLib/Casc/Common.hpp"

using namespace std::experimental::filesystem;

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cout << "Usage: casc <location> <mode> <key> [<output_name>]" << std::endl;
        return 0;
    }

    try
    {
        Casc::CascContainer container(argv[1], {
            std::make_shared<Casc::ZlibHandler<>>()
        });

        try
        {
            std::shared_ptr<Casc::CascStream<false>> file;
            
            switch (*argv[2])
            {
            case '0':
                file = container.openFileByKey(argv[3]);
                break;

            case '1':
                file = container.openFileByHash(argv[3]);
                break;

            case '2':
                file = container.openFileByName(argv[3]);
                break;

            default:
                std::cout << "Invalid mode." << std::endl;
                return -1;
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
                std::unique_ptr<char[]> arr = std::make_unique<char[]>(size);
                file->read(arr.get(), size);

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
        catch (FileNotFoundException ex)
        {
            std::stringstream ss;
            
            ss << "Couldn't find a file with the given key (" << ex.key << ").";

            std::cout << ss.str() << std::endl;
            return -1;
        }
    }
    catch (CascException ex)
    {
        std::stringstream ss;

        ss << "Failed to open the CASC container (" << ex.what() << ").";

        std::cout << ss.str() << std::endl;
        return -1;
    }

    return 0;
}