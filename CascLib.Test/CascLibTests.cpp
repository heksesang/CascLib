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
#include "../CascLib/Shmem.hpp"

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
			auto container = std::make_unique<CascContainer>(R"(I:\World of Warcraft\)");
			auto root = container->openFileByKey(container->buildConfig()["root"].front());
			//auto root = container->findFile("16B35FCCABC6BE82DD");
			

			/*root.seekg(0, std::ios_base::end);
			auto size = root.tellg();

			auto data = std::make_unique<char[]>(size);

			root.seekg(0, std::ios_base::beg);
			root.read(data.get(), size);

			std::ofstream fs;
			fs.open("root", std::ios_base::out | std::ios_base::binary);

			fs.write(data.get(), size);

			fs.close();*/
		}

		TEST_METHOD(GetEncodingFile)
		{
			auto container = std::make_unique<CascContainer>(R"(I:\World of Warcraft\)");

			/*MemoryInfo loc;
			loc.file_ = 28;
			loc.offset_ = 368053824;
			loc.size_ = 52535596;*/

			//auto encoding = container->openStream(loc);
            auto encoding = container->openFileByKey(container->buildConfig()["encoding"].back());
			CascEncoding enc(&encoding);

			auto key = enc.findKey(container->buildConfig()["root"].front());
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
            Shmem shmem(R"(shmem)", R"(I:\World of Warcraft\)");
		}

	};
}