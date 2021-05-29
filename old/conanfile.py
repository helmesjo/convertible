# coding=utf8

from conans import ConanFile, CMake
import tempfile

class Convertible(ConanFile):
    name = "convertible"
    version = "0.1"
    description = "Type mapping and automatic conversion"
    topics = ("conan", "convertible", "conversion", "type-mapping", "binding")
    author = "Fred Helmesjö <helmesjo@gmail.com>"
    url = "https://github.com/helmesjo/convertible.git"
    license = "MIT"
    exports_sources = "*", "!build", "LICENSE", "README.*"

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "build_tests": [True, False]
    }
    default_options = {
        "build_tests": True
    }

    def build(self):
        cmake = CMake(self, set_cmake_flags=True)
        cmake.definitions["CONVERTIBLE_BUILD_TESTS"] = self.options.build_tests
        cmake.configure(build_dir="build")
        cmake.build(build_dir="build")

        if self.options.build_tests:
            cmake.test(build_dir="build")

    def package(self):
        self.copy("include/*.hpp")
    def package_id(self):
        self.info.header_only()
