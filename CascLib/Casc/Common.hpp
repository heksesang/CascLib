#pragma once

#include <fstream>

namespace Casc
{
    /* Container */
    class CascContainer;

    /* BLTE handler */
    template <typename Traits>
    class BaseCascBlteHandler;
    typedef BaseCascBlteHandler<std::filebuf::traits_type> CascBlteHandler;
    
    /* Buffer */
    template <size_t BufferSize>
    class BaseCascBuffer;
    typedef BaseCascBuffer<4096U> CascBuffer;
    
    class CascBuildInfo;
    class CascConfiguration;
    class CascEncoding;
    class CascIndex;
    struct CascIndexRecord;
    class CascRootHandler;
    class CascShmem;
    class CascStream;
}

#include "StreamOps.hpp"

#include "Exceptions/CascException.hpp"
#include "Exceptions/FileNotFoundException.hpp"
#include "Exceptions/InvalidHashException.hpp"
#include "Exceptions/InvalidMagicException.hpp"

using namespace Casc::Exceptions;

#include "Shared/BufferInfo.hpp"
#include "Shared/ChunkInfo.hpp"
#include "Shared/FileSearch.hpp"
#include "Shared/Functions.hpp"
#include "Shared/Hex.hpp"

#include "DataTypes/CompressionMode.hpp"
#include "DataTypes/MemoryInfo.hpp"
#include "DataTypes/Reference.hpp"

#include "CascBlteHandler.hpp"
#include "CascBuffer.hpp"
#include "CascBuildInfo.hpp"
#include "CascConfiguration.hpp"
#include "CascContainer.hpp"
#include "CascEncoding.hpp"
#include "CascIndex.hpp"
#include "CascRootHandler.hpp"
#include "CascShmem.hpp"
#include "CascStream.hpp"