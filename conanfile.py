from conans import ConanFile

class HelloConan(ConanFile):
    name = "CascLib"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    url="https://github.com/heksesang/CascLib"
    license="GPL v3"

    def source(self):
        self.run("git clone https://github.com/heksesang/CascLib.git")

    def requirements(self):
        self.requires.add("zlib/1.2.8@lasote/stable", private=False)

    def package(self):
        self.copy("*.hpp", dst="include", src="CascLib/CascLib")

    def package_info(self):
        self.cpp_info.libs = []