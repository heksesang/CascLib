#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <fstream>
#include <memory>
#include <thread>
#include <vector>

#include "../CascLib/Casc/Common.hpp"

using namespace Casc;
 
namespace CascLibTest
{
	TEST_CLASS(CascLibTests)
	{
	public:

        TEST_METHOD(LoadContainer)
        {
            auto container = std::make_unique<CascContainer>(
                R"(I:\Diablo III\)",
                std::vector<std::shared_ptr<CascBlteHandler>> {
                    std::make_shared<ZlibHandler>()
            });
        }

		TEST_METHOD(GetRootFile)
		{
            auto container = std::make_unique<CascContainer>(
                R"(I:\Diablo III\)",
                std::vector<std::shared_ptr<CascBlteHandler>> {
                    std::make_shared<ZlibHandler>()
            });
			auto root = container->openFileByHash(container->buildConfig()["root"].front());
            
            root->seekg(0, std::ios_base::end);
            auto size = root->tellg();

            std::fstream fs;
            char* arr;

            char magic[4];
            int count;

            root->seekg(0, std::ios_base::beg);
            root->read(magic, 4);
            root->read((char*)&count, 4);

            for (int i = 0; i < count; ++i)
            {
                std::array<uint8_t, 16> hash;
                std::string name;

                root->read(reinterpret_cast<char*>(&hash[0]), 16);
                std::getline(*root.get(), name, '\0');

                Hex<16> hex(hash);

                try
                {
                    auto dir = container->openFileByHash(hex.string());
                    dir->seekg(0, std::ios_base::end);
                    auto dirSize = dir->tellg();

                    dir->seekg(0, std::ios_base::beg);
                    arr = new char[(size_t)dirSize];

                    dir->read(arr, dirSize);

                    fs.open(name, std::ios_base::out | std::ios_base::binary);

                    fs.write(arr, dirSize);
                    fs.close();

                    delete[] arr;
                }
                catch (...)
                {
                    continue;
                }
            }

            root->seekg(0, std::ios_base::beg);
            arr = new char[(size_t)size];

            root->read(arr, size);

            fs.open("root.out", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;


		}

		TEST_METHOD(GetEncodingFile)
		{
            auto container = std::make_unique<CascContainer>(
                R"(I:\Diablo III\)",
                std::vector<std::shared_ptr<CascBlteHandler>> {
                    std::make_shared<ZlibHandler>()
            });
			/*CascEncoding enc(
                container->openFileByKey(container->buildConfig()["encoding"].back()));

			auto key = enc.findKey(container->buildConfig()["root"].front());*/

            auto enc = container->openFileByKey(container->buildConfig()["encoding"].back());

            enc->seekg(0, std::ios_base::end);
            auto size = enc->tellg();

            enc->seekg(0, std::ios_base::beg);
            auto arr = new char[(size_t)size];

            enc->read(arr, size);

            std::fstream fs;
            fs.open("enc.out", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;
		}

		TEST_METHOD(ReadConfiguration)
		{
			CascConfiguration configuration(R"(I:\Diablo III\Data\config\0d\a0\0da08d69484c74c91e50aab485f5b4ba)");
		}

		TEST_METHOD(ReadBuildInfo)
		{
			CascBuildInfo buildInfo(R"(I:\Diablo III\.build.info)"); 
		}

		TEST_METHOD(ReadShmem)
		{
            CascShmem shmem(R"(shmem)", R"(I:\Diablo III\)");
		}

        TEST_METHOD(WriteFile)
        {
            auto container = std::make_unique<CascContainer>(
                R"(I:\Diablo III\)",
                std::vector<std::shared_ptr<CascBlteHandler>> {
                    std::make_shared<ZlibHandler>()
            });

            auto enc = container->openFileByKey(container->buildConfig()["encoding"].back());

            enc->seekg(0, std::ios_base::end);
            auto size = enc->tellg();

            enc->seekg(0, std::ios_base::beg);

            container->write(*enc.get(), CascLayoutDescriptor({
                CascChunkDescriptor(CompressionMode::None, 0, (size_t)size) }));
        }

	};
}