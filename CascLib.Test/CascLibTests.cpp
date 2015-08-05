#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <fstream>
#include <memory>
#include <thread>
#include <vector>

#include "../CascLib/CascConfiguration.hpp"
#include "../CascLib/CascBuildInfo.hpp"
#include "../CascLib/CascContainer.hpp"
#include "../CascLib/CascEncoding.hpp"
#include "../CascLib/CascShmem.hpp"

using namespace Casc;
 
namespace CascLibTest
{
	TEST_CLASS(CascLibTests)
	{
	public:

        TEST_METHOD(LoadContainer)
        {
            auto container = std::make_unique<CascContainer>(R"(I:\World of Warcraft\)");
        }

		TEST_METHOD(GetRootFile)
		{
			auto container = std::make_unique<CascContainer>(R"(I:\Diablo III\)");
			auto root = container->openFileByHash(container->buildConfig()["root"].front());
            
            root->seekg(0, std::ios_base::end);
            auto size = root->tellg();

            root->seekg(0, std::ios_base::beg);
            auto arr = new char[size];

            root->read(arr, size);

            std::fstream fs;
            fs.open("root.out", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;
		}

		TEST_METHOD(GetEncodingFile)
		{
			auto container = std::make_unique<CascContainer>(R"(I:\Diablo III\)");
			/*CascEncoding enc(
                container->openFileByKey(container->buildConfig()["encoding"].back()));

			auto key = enc.findKey(container->buildConfig()["root"].front());*/

            auto enc = container->openFileByKey(container->buildConfig()["encoding"].back());

            enc->seekg(0, std::ios_base::end);
            auto size = enc->tellg();

            enc->seekg(0, std::ios_base::beg);
            auto arr = new char[size];

            enc->read(arr, size);

            std::fstream fs;
            fs.open("enc.out", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;
		}

		TEST_METHOD(ReadConfiguration)
		{
			CascConfiguration configuration("config");
		}

		TEST_METHOD(ReadBuildInfo)
		{
			CascBuildInfo buildInfo(".build.info"); 
		}

		TEST_METHOD(ReadShmem)
		{
            CascShmem shmem(R"(shmem)", R"(I:\World of Warcraft\)");
		}

	};
}