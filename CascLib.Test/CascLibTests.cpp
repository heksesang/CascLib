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

        TEST_METHOD(GetBucket)
        {
            std::vector<uint8_t> vec{ 0x41, 0xEE, 0x19, 0x86, 0xAC, 0xC5, 0x33, 0xCC, 0x00 };
            std::array<uint8_t, 9> arr{ 0x41, 0xEE, 0x19, 0x86, 0xAC, 0xC5, 0x33, 0xCC, 0x00 };
        }

        TEST_METHOD(ReadBuildInfo)
        {
            Parsers::Text::BuildInfo buildInfo(R"(I:\Diablo III\.build.info)");
        }

        TEST_METHOD(ReadConfiguration)
        {
            Parsers::Text::BuildInfo buildInfo(R"(I:\Diablo III\.build.info)");

            auto buildConfigHash = buildInfo.build(0).at("Build Key");

            IO::StreamAllocator alloc(R"(I:\Diablo III\Data)");

            Parsers::Text::Configuration configuration(
                alloc.config<true, false>(buildInfo.build(0).at("Build Key")));
        }

        TEST_METHOD(ReadShmem)
        {
            Parsers::Binary::ShadowMemory shadowMemory(R"(I:\Diablo III\Data\data\shmem)");
        }

        TEST_METHOD(LoadContainer)
        {
            auto container = std::make_unique<Container>(
                R"(I:\Overwatch\)",
                R"(data\casc)");
        }

        TEST_METHOD(GetFileByKey)
        {
            auto container = std::make_unique<Container>(
                R"(I:\Overwatch\)",
                R"(data\casc)");

            Parsers::Text::BuildInfo buildInfo(R"(I:\Overwatch\.build.info)");

            IO::StreamAllocator alloc(R"(I:\Overwatch\data\casc)");

            Parsers::Text::Configuration configuration(
                alloc.config<true, false>(buildInfo.build(0).at("Build Key")));

            auto file = container->openFileByKey(configuration["encoding"].back(), "");

            file->seekg(0, std::ios_base::end);
            auto size = file->tellg();

            file->seekg(0, std::ios_base::beg);
            auto arr = new char[(size_t)size];

            file->read(arr, size);

            std::fstream fs;
            fs.open("enc.ow", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;
        }

		TEST_METHOD(GetFileByHash)
		{
            auto container = std::make_unique<Container>(
                R"(I:\Overwatch)",
                R"(data\casc)");

            Parsers::Text::BuildInfo buildInfo(R"(I:\Overwatch\.build.info)");

            IO::StreamAllocator alloc(R"(I:\Overwatch\data\casc)");

            Parsers::Text::Configuration configuration(
                alloc.config<true, false>(buildInfo.build(0).at("Build Key")));

			auto file = container->openFileByHash(configuration["root"].front());
            
            file->seekg(0, std::ios_base::end);
            auto size = file->tellg();

            std::fstream fs;
            char* arr;

            char magic[4];
            int count;

            file->seekg(0, std::ios_base::beg);
            file->read(magic, 4);
            file->read((char*)&count, 4);

            file->seekg(0, std::ios_base::beg);
            arr = new char[(size_t)size];

            file->read(arr, size);

            fs.open("root.ow", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;
		}

        TEST_METHOD(WriteFile)
        {
            auto container = std::make_unique<Container>(
                R"(I:\Diablo III\)",
                "Data");

            unsigned char hexData[10] = {
                0xDE, 0xAD, 0xBE, 0xEF, 0x3D, 0x60, 0x0D, 0xF0, 0x0D, 0x00
            };
            char* data = reinterpret_cast<char*>(hexData);

            auto out = container->write();

            out->write(data, 5);
            out->setMode(IO::EncodingMode::Zlib);
            out->write(data + 5, 5);
            out->close();
        }

	};
}