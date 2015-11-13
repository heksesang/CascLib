## CascLib

CascLib is C++ library that allows you to access files from a CASC library from any Blizzard game.
It aims for a modern C++14 design, being header-only, and leveraging standard C++ features like streams.

### CASC

CASC stands for "Content Addressable Storage Container".
It is a replacement for the older MPQ format used by Blizzard in previous game titles.

### Features

* Header-only library.
* Look up files based on either file key, file content hash, or filename.
* Read files from any CASC archive.
* Write files to any CASC archive.
* Apply patches to any CASC archive.

### Implemented features

* Look up files based on either file key or file content hash in any CASC archive.
* Look up files based on filename in WoW CASC archives.
* Read files from any non-Overwatch CASC archive.

### Features currently being implemented

* Look up files based on filename in Diablo III, Heroes of the Storm, Starcraft II and Overwatch.
* Encryption support (needed for Overwatch support at the time of writing).
* Write files.
* Apply patches.

### Requirements

* GCC 5, Clang 3.6 or Visual Studio 2015.
* Zlib
* Boost Filesystem (not required for Visual Studio 2015).

### How to use

1. Include Casc/Common.hpp in your project.
2. Include and link to zlib and boost filesystem (if not compiling with Visual Studio 2015).

**Example**
``` c++
    #include "Casc/Common.hpp"
    
    void doStuffWithFile(std::istream& stream)
    {
        // Do stuff with the stream...
    }

    int main(int argc, char* argv[])
    {
        // Init the container, specifying the path and handlers.
        Casc::Container container(
            R"(C:\Program Files (x86)\World of Warcraft)",
            R"(Data)"
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
