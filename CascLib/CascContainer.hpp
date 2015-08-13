#pragma once

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "CascShmem.hpp"
#include "CascBlteHandler.hpp"
#include "CascBuildInfo.hpp"
#include "CascConfiguration.hpp"
#include "CascEncoding.hpp"
#include "CascIndex.hpp"
#include "CascStream.hpp"
#include "CascTraits.hpp"
#include "CascRootHandler.hpp"
#include "Shared/FileSearch.hpp"
#include "Shared/Utils.hpp"

using namespace std::tr2;

namespace Casc
{
    using namespace Casc::Shared;

    /**
     * A container for a CASC archive.
     */
    template <typename Traits>
    class BaseCascContainer
    {
    public:
        typedef Traits traits_type;

    public:
        std::shared_ptr<CascStream> openFileByKey(const std::string &key) const
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

        std::shared_ptr<CascStream> openFileByHash(const std::string &hash) const
        {
            return openFileByKey(encoding->findKey(hash));
        }

        std::shared_ptr<CascStream> openFileByName(const std::string &name) const
        {
            auto root = container->openFileByHash(this->buildConfig()["root"].front());

            std::array<char, 4> magic;
            root->read(&magic[0], 4);
            root->seekg(0, std::ios_base::beg);

            return openFileByKey(encoding->findKey(rootHandlers[magic]->findHash(name)));
        }

        template <typename T>
        void registerHandler()
        {
            CascBlteHandler* handler = new T;
            this->blteHandlers[handler->compressionMode()] = std::shared_ptr<CascBlteHandler>(handler);
        }

        void registerHandlers(std::vector<std::shared_ptr<CascBlteHandler>> handlers)
        {
            for (std::shared_ptr<CascBlteHandler> handler : handlers)
                this->blteHandlers[handler->compressionMode()] = handler;
        }

        void registerHandlers(std::vector<std::shared_ptr<CascRootHandler>> handlers)
        {
            for (std::shared_ptr<CascRootHandler> handler : handlers)
                this->rootHandlers[handler->fileMagic()] = handler;
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
        CascShmem shmem_;

        // The file indices.
        std::vector<CascIndex> indices_;

        // The encoding file.
        std::unique_ptr<CascEncoding> encoding;

        // Chunk handlers
        std::map<char, std::shared_ptr<CascBlteHandler>> blteHandlers;

        // Chunk handlers
        std::map<std::array<char, 4>, std::shared_ptr<CascRootHandler>> rootHandlers;

        /**
         * Opens a stream at a given location.
         *
         * @param loc	the location of the data to stream.
         * @return		a stream object.
         */
        std::shared_ptr<CascStream> openStream(MemoryInfo &loc) const
        {
            std::stringstream ss;
            ss << shmem_.path() << "/data." << std::setw(3) << std::setfill('0') << loc.file();
            
            return std::make_shared<CascStream>(
                ss.str(), loc.offset(),
                mapToVector(this->blteHandlers));
        }

    public:
        /**
         * Default constructor.
         */
        BaseCascContainer()
        {

        }

        /**
         * Constructor.
         *
         * @param path	the path of the archive folder.
         */
        BaseCascContainer(std::string path,
            std::vector<std::shared_ptr<CascBlteHandler>> blteHandlers = {},
            std::vector<std::shared_ptr<CascRootHandler>> rootHandlers = {})
        {
            registerHandlers(blteHandlers);
            load(path);
        }

        /**
         * Move constructor.
         */
        BaseCascContainer(BaseCascContainer &&) = default;

        /**
         * Move operator.
         */
        BaseCascContainer &BaseCascContainer::operator= (BaseCascContainer &&) = default;

        /**
         * Destructor.
         */
        virtual ~BaseCascContainer()
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

            encoding = std::make_unique<CascEncoding>(openFileByKey(buildConfig_["encoding"].back()));
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

        const CascShmem &shmem() const
        {
            return shmem_;
        }
    };

    typedef BaseCascContainer<CascTraits> CascContainer;
}