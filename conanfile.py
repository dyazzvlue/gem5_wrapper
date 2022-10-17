import re
import sys
import os
from conans import ConanFile, tools, CMake
from os.path import isdir

# python3 `which conan` create . demo/testing


class Gem5WrapperConan(ConanFile):
    name = 'gem5_wrapper'
    description = 'Gem5 TLM wrapper'
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "fPIC": [True, False],
        "CONANPKG": ["ON", "OFF"],
        "shared": [True, False],
        "build_tool": ["cmake", "scons"],
    }
    default_options = {
        "fPIC": True,
        "CONANPKG": "OFF",
        "shared": True,
        "build_tool": "cmake"
    }
    version = "1.0"
    url = "https://gitlab.devtools.intel.com/syssim/cofluent"  # TODO
    license = "Proprietary"  # TODO

    requires = (
        "systemc/2.3.3@syssim/stable",
        "gem5/1.0@demo/testing"
    )

    exports_sources = (
        "include/*",
        "src/*",
        "SConstruct",
        "CMakeLists.txt"
    )

    __cmake = None

    def configure(self):
        if self.options.build_tool == "scons" :
            self.generators = "scons"
        else:
            self.generators = "cmake", "cmake_find_package"

    def build(self):
        if self.options.build_tool == "scons" :
            self.build_scons()
        else:
            self.build_cmake()


    def build_scons(self):
        # build gem5 lib
        self.run('scons -j4')

    def build_cmake(self):
        print("self.source_folder", self.source_folder)
        self._cmake.configure(source_dir=self.source_folder)
        self._cmake.build()
        #if tools.get_env("CONAN_RUN_TESTS", True):
        #    self._cmake.test()
    
    @property
    def _cmake(self):
        if self.__cmake is None:
            self.__cmake = CMake(self)
        return self.__cmake

    def package(self):
        self.copy("*.hh", dst = "include", src="include")
        self.copy("*.h", dst = "include", src="include")
        self.copy("*.a", dst = "lib", keep_path=False)
        self.copy("*.so", dst = "lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = "gem5_wrapper"