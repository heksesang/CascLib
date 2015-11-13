#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <fstream>
#include <memory>
#include <thread>
#include <vector>
#include <experimental/filesystem>

#include "../CascLib/Casc/IO/Handler.hpp"
#include "../CascLib/Casc/IO/Buffer.hpp"
#include "../CascLib/Casc/IO/Stream.hpp"
#include "../CascLib/Casc/Common.hpp"

using namespace Casc;
 
namespace CascLibTest
{

    std::vector<char> noneData;
    std::vector<char> zData;

	TEST_CLASS(CascLibTests)
	{
	public:

        TEST_CLASS_INITIALIZE(Setup)
        {
            noneData = std::vector<char>({ '\x42', '\x4C', '\x54', '\x45', '\x00', '\x00', '\x00', '\x3C', '\x0F', '\x00', '\x00', '\x02', '\x00', '\x00', '\x00', '\x05',
                '\x00', '\x00', '\x00', '\x04', '\xD5', '\x42', '\x8E', '\x3D', '\xF8', '\xCC', '\x05', '\xE8', '\xC9', '\xF0', '\x91', '\xC4',
                '\x0D', '\x76', '\x36', '\x95', '\x00', '\x00', '\x00', '\x05', '\x00', '\x00', '\x00', '\x04', '\xE5', '\x02', '\x60', '\x09',
                '\xBF', '\x23', '\xEA', '\x64', '\xF8', '\x8A', '\x7E', '\x1C', '\xF4', '\xEC', '\x0E', '\x17', '\x4E', '\x74', '\x65', '\x73',
                '\x74', '\x4E', '\x72', '\x65', '\x73', '\x74' });
            zData = std::vector<char>({ '\x42', '\x4C', '\x54', '\x45', '\x00', '\x00', '\x00', '\x24', '\x0F', '\x00', '\x00', '\x01', '\x00', '\x00', '\x00', '\x0D',
                '\x00', '\x00', '\x00', '\x04', '\xE3', '\xF8', '\x49', '\xB6', '\xC2', '\xB6', '\x06', '\x9C', '\xD7', '\xE8', '\xD3', '\x23',
                '\x13', '\xDC', '\xDE', '\x42', '\x5A', '\x78', '\xDA', '\x2B', '\x49', '\x2D', '\x2E', '\x01', '\x00', '\x04', '\x5D', '\x01',
                '\xC1' });

            std::fstream fs;

            fs.open("none.bin", std::ios_base::out | std::ios_base::binary);
            fs.write(reinterpret_cast<char*>(noneData.data()), noneData.size());
            fs.close();

            fs.open("zlib.bin", std::ios_base::out | std::ios_base::binary);
            fs.write(reinterpret_cast<char*>(zData.data()), zData.size());
            fs.close();
        }

        TEST_CLASS_CLEANUP(Cleanup)
        {
            noneData.clear();
            std::experimental::filesystem::remove("none.bin");

            zData.clear();
            std::experimental::filesystem::remove("zlib.bin");
        }

        TEST_METHOD(NoneHandler)
        {
            IO::Chunk chunk
            {
                4,
                8,
                5,
                5
            };

            auto source = std::make_shared<IO::Impl::MemoryMappedSource>(
                std::vector<char>{ noneData.begin() + 60 + chunk.offset, noneData.begin() + 60 + chunk.offset + chunk.size });
            auto handler = std::make_shared<IO::Impl::NoneHandler>(chunk, source);

            auto decoded = handler->decode(0, chunk.size - 1);

            auto equal = std::memcmp(decoded.data(), noneData.data() + 60 + chunk.offset + 1, chunk.size - 1);
            Assert::AreEqual(0, equal);
        }

        TEST_METHOD(ZlibHandler)
        {
            IO::Chunk chunk
            {
                0,
                4,
                0,
                zData.size()
            };

            auto source = std::make_shared<IO::Impl::MemoryMappedSource>(
                std::vector<char>{ zData.begin() + 36 + chunk.offset, zData.begin() + 36 + chunk.offset + chunk.size });
            auto handler = std::make_shared<IO::Impl::ZlibHandler>(chunk, source);

            auto decoded = handler->decode(0, chunk.end - chunk.begin);
            decoded = handler->decode(0, chunk.end - chunk.begin);

            auto equal = std::memcmp(decoded.data(), noneData.data() + 60 + chunk.offset + 1, chunk.end - chunk.begin);
            Assert::AreEqual(0, equal);
        }

        TEST_METHOD(NoneHandlerWithStream)
        {
            IO::Chunk chunk
            {
                4,
                8,
                5,
                5
            };

            auto stream = std::make_shared<std::ifstream>("none.bin", std::ios_base::in | std::ios_base::binary);
            auto source = std::make_shared<IO::Impl::StreamSource>(stream, std::make_pair(65U, 70U));
            auto handler = std::make_shared<IO::Impl::NoneHandler>(chunk, source);

            auto decoded = handler->decode(0, chunk.size - 1);

            auto equal = std::memcmp(decoded.data(), noneData.data() + 60 + chunk.offset + 1, chunk.size - 1);
            Assert::AreEqual(0, equal);
        }

        TEST_METHOD(ZlibHandlerWithStream)
        {
            IO::Chunk chunk
            {
                0,
                4,
                0,
                zData.size()
            };

            auto stream = std::make_shared<std::ifstream>("zlib.bin", std::ios_base::in | std::ios_base::binary);
            auto source = std::make_shared<IO::Impl::StreamSource>(stream, std::make_pair(36U, zData.size() - 36U));

            auto handler = std::make_shared<IO::Impl::ZlibHandler>(chunk, source);

            auto decoded = handler->decode(0, chunk.end - chunk.begin);
            decoded = handler->decode(0, chunk.end - chunk.begin);

            auto equal = std::memcmp(decoded.data(), noneData.data() + 60 + chunk.offset + 1, chunk.end - chunk.begin);
            Assert::AreEqual(0, equal);
        }

        TEST_METHOD(ParseBlockTable)
        {
            auto blockTableSize = IO::Buffer::getBlockTableSize(noneData.begin());
            auto chunks = IO::Buffer::parseBlockTable(noneData.begin() + 8, noneData.begin() + blockTableSize);
            Assert::AreEqual(2U, chunks.size());
        }

        TEST_METHOD(BufferWithNoneHandlers)
        {
            IO::Buffer b;
            b.open("none.bin", 0);

            char arr[8];

            b.sgetn(arr, 8);
            auto equal = std::memcmp(arr, noneData.data() + 60 + 1, 4);
            Assert::AreEqual(0, equal);
            equal = std::memcmp(arr + 4, noneData.data() + 60 + 6, 4);
            Assert::AreEqual(0, equal);
        }

        TEST_METHOD(GetFileSize)
        {
            IO::Buffer b;
            b.open("none.bin", 0);
            auto pos = b.pubseekoff(0, std::ios_base::end);

            Assert::AreEqual(8U, (size_t)pos);
        }

        TEST_METHOD(StreamRead)
        {
            IO::Stream stream;
            stream.open("none.bin", 0);

            stream.seekg(0, std::ios_base::end);
            auto size = stream.tellg();
            
            int equal = 0;

            if (size > 0)
            {
                char* arr = new char[(size_t)size];

                stream.seekg(0, std::ios_base::beg);
                stream.read(arr, size);

                equal = std::memcmp(arr, noneData.data() + 60 + 1, 4);

                if (equal == 0)
                    equal = std::memcmp(arr + 4, noneData.data() + 60 + 6, 4);

                delete arr;
            }

            Assert::AreEqual(0, equal);
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
            auto stream = std::make_shared<std::ifstream>(R"(I:\Diablo III\Data\data\shmem)", std::ios_base::in | std::ios_base::binary);
            Parsers::Binary::ShadowMemory shadowMemory(stream);
        }

        TEST_METHOD(LoadContainer)
        {
            auto container = std::make_unique<Container>(
                R"(I:\World of Warcraft)",
                R"(Data)");
        }

        TEST_METHOD(GetFileByKey)
        {
            auto container = std::make_unique<Container>(
                R"(I:\Overwatch)",
                R"(data\casc)");

            Parsers::Text::BuildInfo buildInfo(R"(I:\Overwatch\.build.info)");

            IO::StreamAllocator alloc(R"(I:\Overwatch\data\casc)");

            Parsers::Text::Configuration configuration(
                alloc.config<true, false>(buildInfo.build(0).at("Build Key")));

            auto file = container->openFileByKey(configuration["encoding"].back());

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
                R"(I:\World of Warcraft)",
                R"(Data)");

            Parsers::Text::BuildInfo buildInfo(R"(I:\World of Warcraft\.build.info)");

            IO::StreamAllocator alloc(R"(I:\World of Warcraft\Data)");

            Parsers::Text::Configuration configuration(
                alloc.config<true, false>(buildInfo.build(0).at("Build Key")));

			auto file = container->openFileByHash(configuration["root"].front());
            //auto file = container->openFileByHash("948c12c5b95d8ea92ebf3dc1af003792");
            
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

            fs.open("root.wow", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;
		}

        TEST_METHOD(GetFileByName)
        {
            auto container = std::make_unique<Container>(
                R"(I:\World of Warcraft)",
                R"(Data)");

            auto file = container->openFileByName("SPELLS\\BONE_CYCLONE_STATE.M2");

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

            fs.open("BONE_CYCLONE_STATE.M2", std::ios_base::out | std::ios_base::binary);

            fs.write(arr, size);
            fs.close();

            delete[] arr;
        }

	};
}