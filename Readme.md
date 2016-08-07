## CascLib

CascLib is C++ library that allows you to access files from CASC archives used in recent Blizzard games.
The library has been designed with modern C++ in mind. Leveraging C++17 and standard features as much as possible, the goal to easily integrate with other libraries.

### CASC

CASC stands for "Content Addressable Storage Container".
It is a replacement for the older MPQ format used by Blizzard in previous game titles.

### Features

* Mostly header-only library.
* Look up files based on either file key, file content hash, or filename.
* Read files from any CASC archive.
* Write files to any CASC archive.
* Apply patches to any CASC archive.

### Implemented features

* Look up files based on either file key or file content hash in any CASC archive.
* Look up files based on filename in WoW CASC archives.
* Read files from any non-Overwatch CASC archive.

### Future features

* Encryption support (needed for Overwatch support at the time of writing).
* Reading files from Overwatch CASC archives.
* Look up files based on filename in Diablo III, Heroes of the Storm, Starcraft II and Overwatch.
* Write files.
* Apply patches.

### Requirements

* GCC 6, Clang 3.8 or Visual Studio 2015.
* libstdc++-6 (on linux)
* zlib

### How to use

#### From conan

The package is also hosted on [Conan](https://www.conan.io/).

Add the following package to your project:
```
casc/1.0.0@heksesang/stable
```

#### From source

1. Clone the library.
   ```
   git clone https://github.com/heksesang/CascLib.git
   ```

2. Add the CascLib project folder to your include directories.

3. Add zlib to your include directories and linker.

4. Include "Casc/Common.hpp" to include the Casc namespace.
   ``` c++
   #include "Casc/Common.hpp"
   ```

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
