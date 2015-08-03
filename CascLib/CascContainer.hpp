#pragma once

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "Shmem.hpp"
#include "CascBuildInfo.hpp"
#include "CascConfiguration.hpp"
#include "CascEncoding.hpp"
#include "CascIndex.hpp"
#include "CascStream.hpp"
#include "Shared/FileSearch.hpp"

using namespace std::tr2;

namespace Casc
{
    /**
     * A container for a CASC archive.
     */
    class CascContainer
    {
    public:
        CascStream openFileByKey(const std::string &key) const
        {
            auto bytes = Hex<9>(key).data();

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

    private:
        // The path of the archive folder.
        std::string path_;

        // The build info.
        CascBuildInfo buildInfo_;

        // The build configuration.
        CascConfiguration buildConfig_;

        // The CDN configuration.
        CascConfiguration cdnConfig_;

        // The SHMEM.
        Shmem shmem_;

        // The file indices.
        std::vector<CascIndex> indices_;

        // The encoding file.
        CascStream encodingStream;

        // The encoding parser.
        CascEncoding encoding;

    public:
        /**
         * Opens a stream at a given location.
         *
         * @param loc	the location of the data to stream.
         * @return		a stream object.
         */
        CascStream openStream(MemoryInfo &loc) const
        {
            std::stringstream ss;
            ss << shmem_.path() << "/data." << std::setw(3) << std::setfill('0') << loc.file();

            return CascStream(ss.str(), loc.offset());
        }

    public:
        /**
         * Default constructor.
         */
        CascContainer()
        {

        }

        /**
         * Constructor.
         *
         * @param path	the path of the archive folder.
         */
        CascContainer(std::string path)
        {
            load(path);
        }

        /**
         * Move constructor.
         */
        CascContainer(CascContainer&&) = default;

        /**
         * Move operator.
         */
        CascContainer& CascContainer::operator= (CascContainer &&) = default;

        /**
         * Destructor.
         */
        virtual ~CascContainer()
        {
        }

        /**
         * Loads a CASC archive into this container.
         *
         * @param path	the path of the archive folder.
         */
        void load(std::string path)
        {
            path_ = path;
            buildInfo_.parse(path + ".build.info");

            FileSearch fs({
                buildInfo_.build(0).at("Build Key"),
                buildInfo_.build(0).at("CDN Key"),
                "shmem"
            }, path_);

            buildConfig_.parse(fs.results().at(0));
            cdnConfig_.parse(fs.results().at(1));
            shmem_.parse(fs.results().at(2), path);

            for (size_t i = 0; i < shmem_.versions().size(); ++i)
            {
                std::stringstream ss;

                ss << shmem_.path() << "/";
                ss << std::setw(2) << std::setfill('0') << std::hex << i;
                ss << std::setw(8) << std::setfill('0') << std::hex << shmem_.versions().at(i);
                ss << ".idx";

                indices_.push_back(ss.str());
            }

            encodingStream = openFileByKey(buildConfig_["encoding"].back());
            encoding = CascEncoding(&encodingStream);
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