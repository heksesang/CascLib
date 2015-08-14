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