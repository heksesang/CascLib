#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

//#include "../CascLib/Casc/Common.hpp"

//using namespace Casc;

namespace CascLibTest
{
	TEST_CLASS(CascHelperTests)
	{
	public:

		/*TEST_METHOD(SwapBytes)
		{
            std::array<char, sizeof(uint32_t)> le;
            *reinterpret_cast<uint32_t*>(le.data()) = 3221225406u;

			Assert::AreEqual(3204448191u, read<IO::EndianType::Big, uint32_t>(le.begin()));
		}

        TEST_METHOD(Lookup3HashBytes)
        {
            std::array<char, 16> data{ 0x07, 0x00, 0x00, 0x00, 0x04, 0x05, 0x09, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00 };
            auto hash = Functions::Hash::lookup3(data, 0);

            Assert::AreEqual(1116121890U, hash);
        }

		TEST_METHOD(Lookup3HashString)
		{
			auto hash = Functions::Hash::lookup3(std::string("SPELLS\\BONE_CYCLONE_STATE.M2"), 0);
			Assert::AreEqual(0x92D10BBAu, hash);
		}*/

	};
}