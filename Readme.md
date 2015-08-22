## CascLib

CascLib is C++ library that allows you to access files from a CASC library from any Blizzard game.
It aims for a modern C++14 design, being mostly header-only, and leveraging standard C++ features like streams.

### CASC

CASC stands for "Content Addressable Storage Container".
It is a replacement for the older MPQ format used by Blizzard in previous game titles.

### Features

* Look up files based on either file key, file content hash, or filename.
* Read files from any CASC archive.
* Write files to any CASC archive.
* Apply patches to any CASC archive.

### Implemented features

* Look up files based on either file key or file content hash.
* Reading files.

### Features currently being implemented

* Write files.
* Look up files based on filename.

### Requirements

* GCC 5, Clang 3.6 or Visual Studio 2015.
* Zlib
* Boost Filesystem (not required for Visual Studio 2015).

### How to use

1. Include Casc/Common.hpp in your project.
2. Include and link to zlib and boost filesystem (if not compiling with Visual Studio 2015).

``` c++
    #include "Casc/Common.hpp"
    
    void doStuffWithFile(std::istream& stream)
    {
        // Do stuff with the stream...
    }

    int main(int argc, char* argv[])
    {
        // Init the container, specifying the path and handlers.
        Casc::CascContainer container(
            R"(C:\Program Files (x86)\Diablo III\)",
            std::vector<std::shared_ptr<Casc::CascBlteHandler>> {
                std::make_shared<Casc::ZlibHandler>()
            }
        );
        
        // Open a file by using one of the openFileByXXX methods.
        auto stream = container.openFileByName("SPELLS\\BONE_CYCLONE_STATE.M2");
        
        // Read from the file like you would do with any other std::istream object.
        char signature[4];
        stream->read(signature, 4);
        
        // You can pass it to another function that accepts std::istream.
        doStuffWithFile(*stream.get());
        
        // Close the stream once you're done.
        stream->close();
    }
```

### License

This project is licensed under the GNU General Public License version 3.
