#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <fstream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <experimental/filesystem>

#include "Casc/IO/Handler.hpp"
#include "Casc/IO/Buffer.hpp"
#include "Casc/IO/Stream.hpp"
#include "Casc/Common.hpp"

using namespace Casc;
 
namespace CascLibTest
{
    using namespace std::string_literals;

    std::vector<char> noneData;
    std::vector<char> zData;
    std::string buildInfoData;
    std::string buildConfigData;

    const char* NONE_FILENAME = "5fef29fa35057813d4fcb108d9f25b3b";
    const char* ZLIB_FILENAME = "47bde729d15e8d502669556abdf271f5";
    const char* BUILD_INFO_FILENAME = ".build.info";
    const char* BUILD_CONFIG_FILENAME = "8e374bc49d4f3314a2a4497b065441e3";

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
            buildInfoData = R"(Branch!STRING:0|Active!DEC:1|Build Key!HEX:16|CDN Key!HEX:16|Install Key!HEX:16|IM Size!DEC:4|CDN Path!STRING:0|CDN Hosts!STRING:0|Tags!STRING:0|Armadillo!STRING:0|Last Activated!STRING:0|Version!STRING:0|KeyService!STRING:0
eu|1|da20cf2b7e65e2f2352397b6295e10c0|07d527cb36304d51fd1c6ef689cbca34|74b367347d660c8c5ef6563159f4d7d2|12274|/tpr/d3|blzddist1-a.akamaihd.net blzddist2-a.akamaihd.net|Windows EU? enUS speech?:Windows EU? enUS text?||2016-10-04T16:01:04Z|2.4.2.39192|http://eu.patch.battle.net:1119/d3/keyring
)";
            buildConfigData = R"(# Build Configuration

root = eee756b2f8307b30bad5fd99393d03c9
install = 748b2047d90dc20e0b935a0a49d0e507
install-size = 17862
download = 2747fec970f46a0cf98e971dbca32794
download-size = 32141073
partial-priority = ea0d5b151e2f32dacb48517dd1565a65
partial-priority-size = 0
encoding = 39135a163b3c371c3bb450fd6613f14e 688062e88dea2bf300588519f18bb363
encoding-size = 70889899 70867109
patch = 8712a4f35bc244e6f0ef87e6f41ce57d
patch-size = 6003601
patch-config = 243725e28d4778da17f2cfc1a03cf4d0
build-name = WOW-22996patch7.1.0_Retail
build-playbuild-installer = ngdptool_casc2
build-product = WoW
build-uid = wow)";

            std::fstream fs;

            fs.open(NONE_FILENAME, std::ios_base::out | std::ios_base::binary);
            fs.write(reinterpret_cast<char*>(noneData.data()), noneData.size());
            fs.close();

            fs.open(ZLIB_FILENAME, std::ios_base::out | std::ios_base::binary);
            fs.write(reinterpret_cast<char*>(zData.data()), zData.size());
            fs.close();

            fs.open(BUILD_INFO_FILENAME, std::ios_base::out);
            fs.write(buildInfoData.c_str(), buildInfoData.size());
            fs.close();

            fs.open(BUILD_CONFIG_FILENAME, std::ios_base::out);
            fs.write(buildConfigData.c_str(), buildConfigData.size());
            fs.close();
        }

        TEST_CLASS_CLEANUP(Cleanup)
        {
            noneData.clear();
            std::experimental::filesystem::remove(NONE_FILENAME);

            zData.clear();
            std::experimental::filesystem::remove(ZLIB_FILENAME);

            buildInfoData.assign("");
            std::experimental::filesystem::remove(BUILD_INFO_FILENAME);

            buildConfigData.assign("");
            std::experimental::filesystem::remove(BUILD_CONFIG_FILENAME);
        }

        TEST_METHOD(ReadBuildInfo)
        {
            Parsers::Text::BuildInfo buildInfo(BUILD_INFO_FILENAME);
            
            Assert::IsTrue(buildInfo.size() == 1);
            Assert::IsTrue(buildInfo.build(0).count("Branch") == 1, L"Missing key 'Branch'");
            Assert::AreEqual("eu"s, buildInfo.build(0).at("Branch"), L"Wrong value for key 'Branch'");

            Assert::IsTrue(buildInfo.build(0).count("Active") == 1, L"Missing key 'Active'");
            Assert::AreEqual("1"s, buildInfo.build(0).at("Active"), L"Wrong value for key 'Active'");

            Assert::IsTrue(buildInfo.build(0).count("Build Key") == 1, L"Missing key 'Build Key'");
            Assert::AreEqual("da20cf2b7e65e2f2352397b6295e10c0"s, buildInfo.build(0).at("Build Key"), L"Wrong value for key 'Build Key'");

            Assert::IsTrue(buildInfo.build(0).count("CDN Key") == 1, L"Missing key 'CDN Key'");
            Assert::AreEqual("07d527cb36304d51fd1c6ef689cbca34"s, buildInfo.build(0).at("CDN Key"), L"Wrong value for key 'CDN Key'");

            Assert::IsTrue(buildInfo.build(0).count("Install Key") == 1, L"Missing key 'Install Key'");
            Assert::AreEqual("74b367347d660c8c5ef6563159f4d7d2"s, buildInfo.build(0).at("Install Key"), L"Wrong value for key 'Install Key'");

            Assert::IsTrue(buildInfo.build(0).count("IM Size") == 1, L"Missing key 'IM Size'");
            Assert::AreEqual("12274"s, buildInfo.build(0).at("IM Size"), L"Wrong value for key 'IM Size'");

            Assert::IsTrue(buildInfo.build(0).count("CDN Path") == 1, L"Missing key 'CDN Path'");
            Assert::AreEqual("/tpr/d3"s, buildInfo.build(0).at("CDN Path"), L"Wrong value for key 'CDN Path'");

            Assert::IsTrue(buildInfo.build(0).count("CDN Hosts") == 1, L"Missing key 'CDN Hosts'");
            Assert::AreEqual("blzddist1-a.akamaihd.net blzddist2-a.akamaihd.net"s, buildInfo.build(0).at("CDN Hosts"), L"Wrong value for key 'CDN Hosts'");

            Assert::IsTrue(buildInfo.build(0).count("Tags") == 1, L"Missing key 'Tags'");
            Assert::AreEqual("Windows EU? enUS speech?:Windows EU? enUS text?"s, buildInfo.build(0).at("Tags"), L"Wrong value for key 'Tags'");

            Assert::IsTrue(buildInfo.build(0).count("Armadillo") == 1, L"Missing key 'Armadillo'");
            Assert::AreEqual(""s, buildInfo.build(0).at("Armadillo"), L"Wrong value for key 'Armadillo'");

            Assert::IsTrue(buildInfo.build(0).count("Last Activated") == 1, L"Missing key 'Last Activated'");
            Assert::AreEqual("2016-10-04T16:01:04Z"s, buildInfo.build(0).at("Last Activated"), L"Wrong value for key 'Last Activated'");

            Assert::IsTrue(buildInfo.build(0).count("Version") == 1, L"Missing key 'Version'");
            Assert::AreEqual("2.4.2.39192"s, buildInfo.build(0).at("Version"), L"Wrong value for key 'Version'");

            Assert::IsTrue(buildInfo.build(0).count("KeyService") == 1, L"Missing key 'KeyService'");
            Assert::AreEqual("http://eu.patch.battle.net:1119/d3/keyring"s, buildInfo.build(0).at("KeyService"), L"Wrong value for key 'KeyService'");
        }

        TEST_METHOD(ReadConfiguration)
        {
            Parsers::Text::Configuration configuration(BUILD_CONFIG_FILENAME);

            Assert::IsTrue(configuration.hasKey("root"), L"Missing key 'root'");
            Assert::AreEqual("eee756b2f8307b30bad5fd99393d03c9"s, configuration["root"].at(0), L"Wrong value for key 'root'");

            Assert::IsTrue(configuration.hasKey("install"), L"Missing key 'install'");
            Assert::AreEqual("748b2047d90dc20e0b935a0a49d0e507"s, configuration["install"].at(0), L"Wrong value for key 'install'");
        }

        TEST_METHOD(ReadShmem)
        {
            auto stream = std::make_shared<std::ifstream>(R"(I:\Diablo III\Data\data\shmem)", std::ios_base::in | std::ios_base::binary);
            Parsers::Binary::ShadowMemory shadowMemory(stream);
        }

        TEST_METHOD(GetFileSize)
        {
            IO::Buffer b;
            b.open(NONE_FILENAME, 0);
            auto pos = b.pubseekoff(0, std::ios_base::end);

            Assert::AreEqual(8U, (size_t)pos);
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
            /*auto blockTableSize = IO::Buffer::getBlockTableSize(noneData.begin());
            auto chunks = IO::Buffer::parseBlockTable(noneData.begin() + 8, noneData.begin() + blockTableSize);
            Assert::AreEqual(2U, chunks.size());*/
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