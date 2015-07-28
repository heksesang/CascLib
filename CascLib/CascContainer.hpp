#pragma once

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "Shmem.hpp"
#include "CascBuildInfo.hpp"
#include "CascConfiguration.hpp"
#include "CascIndex.hpp"
#include "CascStream.hpp"
#include "FileSearch.hpp"

using namespace std::tr2;

namespace Casc {
	class CascContainer
	{
		std::string path_;
		CascBuildInfo buildInfo_;
		CascConfiguration buildConfig_;
		CascConfiguration cdnConfig_;
		Shmem shmem_;
		std::vector<CascIndex> indices_;
	public:
		CascStream openStream(MemoryInfo &loc) const
		{
			std::stringstream ss;
			ss << shmem_.path() << "/data." << std::setw(3) << std::setfill('0') << loc.file();

			return CascStream(ss.str(), loc.offset());
		}

	public:
		CascContainer(std::string path)
			: path_(path),
			  buildInfo_(path + ".build.info"),
			  buildConfig_(FileSearch(buildInfo_.build(0).at("Build Key"), path_).results().at(0)),
			  cdnConfig_(FileSearch(buildInfo_.build(0).at("CDN Key"), path_).results().at(0)),
			  shmem_(FileSearch("shmem", path_).results().at(0), path)
		{
			/*for (size_t i = 0; i < shmem_.versions().size(); ++i)
			{
				std::stringstream ss;

				ss << shmem_.path() << "/";
				ss << std::setw(2) << std::setfill('0') << std::hex << i;
				ss << std::setw(8) << std::setfill('0') << std::hex << shmem_.versions().at(i);
				ss << ".idx";

				indices_.push_back(ss.str());
			}*/
		}

		CascStream findFile(const std::string &key) const
		{
			auto bytes = Hex::fromString<9>(key);

			for (auto index : indices_)
			{
				try
				{
					return openStream(index.file(bytes));
				}
				catch (std::exception&) // TODO: Better exception handling
				{
					continue;
				}
			}

			throw std::exception("File not found");
		}

		const std::string &path() const
		{
			return path_;
		}

		const CascBuildInfo &buildInfo() const
		{
			return buildInfo_;
		}

		const CascConfiguration &buildConfig() const
		{
			return buildConfig_;
		}

		const CascConfiguration &cdnConfig() const
		{
			return cdnConfig_;
		}

		const Shmem &shmem() const
		{
			return shmem_;
		}
	};
}