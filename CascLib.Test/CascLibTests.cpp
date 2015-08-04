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
			auto root = container->openFileByHash(container->buildConfig()["root"].back());
		}

		TEST_METHOD(GetEncodingFile)
		{
			auto container = std::make_unique<CascContainer>(R"(I:\World of Warcraft\)");
			CascEncoding enc(
                container->openFileByKey(container->buildConfig()["encoding"].back()));

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