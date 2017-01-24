from conans import ConanFile
import shutil
import os

def include_hpp(folder, files):

    ignore_list = []
    for file in files:
        full_path = os.path.join(folder, file)
        if not os.path.isdir(full_path):
            filename, file_extension = os.path.splitext(full_path)
            if file_extension != ".hpp":
                ignore_list.append(file)
    return ignore_list

class HelloConan(ConanFile):
    name = "CascLib"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "visual_studio", "gcc"
    url="https://github.com/heksesang/CascLib"
    license="GPL v3"

    def source(self):
        shutil.rmtree(os.getcwd() + "\\CascLib", True)
        shutil.copytree("C:\\Users\\ginnu\\Source\\Repos\\CascLib\\CascLib\\Casc", os.getcwd() + "\\CascLib\\CascLib\\Casc", ignore=include_hpp)
        # self.run("git clone https://github.com/heksesang/CascLib.git")

    def requirements(self):
        self.requires.add("zlib/1.2.8@heksesang/stable", private=False)

    def build(self):
        pass

    def package(self):
        self.copy("*.hpp", dst="include", src="CascLib/CascLib")

    def package_info(self):
        self.cpp_info.libs = []
        if self.settings.compiler == "Visual Studio":
            self.cpp_info.cppflags = ["/std:c++latest"]
