#pragma once

#include <array>
#include <fstream>
#include <memory>
#include <utility>
#include <vector>
#include <stdint.h>

namespace Casc
{
	namespace Internal
	{
		extern "C"
		{
			void hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);
			uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
		}
	}

	namespace Hash
	{
		inline std::pair<uint32_t, uint32_t> lookup3(const std::string &data, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
		{
			auto pc = init.first;
			auto pb = init.second;

			Casc::Internal::hashlittle2(data.c_str(), data.size() + 1, &pc, &pb);

			return std::make_pair(pc, pb);
		}

		inline std::pair<uint32_t, uint32_t> lookup3(const std::vector<char> &data, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
		{
			auto pc = init.first;
			auto pb = init.second;

			Casc::Internal::hashlittle2(data.data(), data.size(), &pc, &pb);

			return std::make_pair(pc, pb);
		}

		inline std::pair<uint32_t, uint32_t> lookup3(std::ifstream &stream, uint32_t length, const std::pair<uint32_t, uint32_t> &init = { 0, 0 })
		{
			auto pc = init.first;
			auto pb = init.second;
			std::array<char, 12> buffer;

			for (unsigned int i = 0; i < length; i += 12)
			{
				stream.read(buffer.data(), buffer.size());
				Casc::Internal::hashlittle2(buffer.data(), (size_t)stream.gcount(), &pc, &pb);
			}

			return std::make_pair(pc, pb);
		}

		inline uint32_t lookup3(const std::string &data, const uint32_t &init)
		{
			return Casc::Internal::hashlittle(data.data() + 1, data.size(), init);
		}

		inline uint32_t lookup3(const std::vector<char> &data, const uint32_t &init)
		{
			return Casc::Internal::hashlittle(data.data(), data.size(), init);
		}

		inline uint32_t lookup3(std::ifstream &stream, uint32_t length, const uint32_t &init)
		{
			auto hash = init;
			std::array<char, 12> buffer;

			auto pos = stream.tellg();

			for (unsigned int i = 0; i < length; i += 12)
			{
				stream.read(buffer.data(), buffer.size());
				hash = Casc::Internal::hashlittle(buffer.data(), (size_t)stream.gcount(), hash);
			}

			stream.seekg(pos);

			return hash;
		}
	}
}