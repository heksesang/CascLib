/*
* Copyright 2016 Gunnar Lilleaasen
*
* This file is part of CascLib.
*
* CascLib is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* CascLib is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CascLib.  If not, see <http://www.gnu.org/licenses/>.
*/

namespace Casc
{
    namespace IO
    {
        namespace Impl
        {
            /**
             * TODO: Needs implementation.
             */
            class CryptHandler : public Handler
            {
			public:
				EncodingMode mode() const override
				{
					return EncodingMode::Crypt;
				}

				std::vector<char> decode(size_t offset, size_t count) override
				{
					return std::vector<char>();
				}

				std::vector<char> encode(std::vector<char> input) const override
				{
					return std::vector<char>();
				}

				size_t logicalSize() override
				{
					return 0;
				}

				void reset() override
				{

				}

				CryptHandler(std::shared_ptr<DataSource> source) :
					Handler(Chunk(), source)
				{

				}

				using Handler::Handler;
			};
        }
    }
}
