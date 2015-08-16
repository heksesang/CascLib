#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include "../CascLib/Casc/Common.hpp"

using namespace Casc;

namespace CascLibTest
{
	TEST_CLASS(CascStreamTests)
	{
	public:
		
		TEST_METHOD(ReadFileSize)
		{
			CascStream<false> fs;
			fs.open(R"(data)", 3656);

			fs.seekg(0, std::ios_base::end);
			auto size = fs.tellg();
			fs.seekg(0, std::ios_base::beg);

			fs.close();

			Assert::IsTrue(size == std::streamoff(16634));
			Assert::IsFalse(fs.fail());
		}

		TEST_METHOD(ReadFile)
		{
			CascStream<false> fs;
			fs.open(R"(data)", 3656);

			std::vector<char> data;
			data.resize(16634);

			fs.seekg(0, std::ios_base::beg);
			fs.read(&data[0], 16634);

			std::ofstream out;
			out.open("data3", std::ios_base::out | std::ios_base::binary);
			out.write(&data[0], 16634);
			out.close();

			fs.close();

			Assert::IsFalse(fs.fail());
			Assert::IsFalse(out.fail());
		}

		TEST_METHOD(SeekFile)
		{
			CascStream<false> fs;
			fs.open(R"(data)", 3656);

			char data[4];
			char data2[4];

			fs.read(data, 4);
			fs.read(data2, 4);

			fs.seekg(4, std::ios_base::cur);

			auto size = fs.tellg();
			std::streamoff off = size;

			fs.close();

			Assert::IsTrue(std::streamoff(12) == size);
			Assert::IsFalse(fs.fail());
		}

	};
}