#include <fstream>
#include <iostream>
#include <sstream>
#include "../CascLib/Casc/Common.hpp"

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
        Casc::CascContainer container(argv[1], {
            std::make_shared<Casc::ZlibHandler>()
        });

        try
        {
            std::shared_ptr<Casc::CascStream<false>> file;
            
            if (strcmp(argv[2], "key") == 0)
            {
                file = container.openFileByKey(argv[3]);
            }
            else if (strcmp(argv[2], "hash") == 0)
            {
                file = container.openFileByHash(argv[3]);
            }
            else if (strcmp(argv[2], "filename") == 0)
            {
                file = container.openFileByName(argv[3]);
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
