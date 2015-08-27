#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include "../CascLib/Casc/Common.hpp"

using namespace Casc::Shared;
using namespace Casc::Shared::Functions;
using namespace Casc::Shared::Functions::Endian;
using namespace Casc::Shared::Functions::Hash;

namespace CascLibTest
{
	TEST_CLASS(CascHelperTests)
	{
	public:
		
		/*TEST_METHOD(ReadMemoryInfo)
		{
			MemoryInfo ref(5, 3221225406, 65);

			Assert::AreEqual(22, ref.file());
			Assert::AreEqual((size_t)1073741758u, ref.offset());
			Assert::AreEqual((size_t)65u, ref.size());
		}*/

		TEST_METHOD(SwapBytes)
		{
            std::array<char, sizeof(uint32_t)> le;
            *reinterpret_cast<uint32_t*>(le.data()) = 3221225406u;

			Assert::AreEqual(3204448191u, read<EndianType::Big, uint32_t>(le.begin(), le.end()));
		}

        TEST_METHOD(HashBytes)
        {
            std::array<char, 16> data{ 0x07, 0x00, 0x00, 0x00, 0x04, 0x05, 0x09, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00 };
            auto hash = lookup3(data, 0);

            Assert::AreEqual(1116121890U, hash);
        }

		TEST_METHOD(HashFilename)
		{
			auto hash = lookup3("SPELLS\\BONE_CYCLONE_STATE.M2", 0);
			Assert::AreEqual(0x502501AAu, hash);
		}

	};
}