from conan.packager import ConanMultiPackager
import os

if __name__ == "__main__":
    os.environ["MINGW_CONFIGURATIONS"] = '6.2@x86_64@seh@posix, 6.2@x86_64@seh@win32'
    os.environ["CONAN_VISUAL_VERSIONS"] = '14'
    builder = ConanMultiPackager(username="heksesang", channel="stable", reference="CascLib/1.0.0", upload=False)
    builder.add_common_builds()
    builder.run()
