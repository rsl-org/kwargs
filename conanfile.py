from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.files import copy


class KwargsRecipe(ConanFile):
    name = "kwargs"
    version = "0.1"
    package_type = "header-library"

    # Optional metadata
    license = "MIT"
    author = "Tsche che@pydong.org"
    description = "A single header library that implements keyword arguments for C++ using C++26 reflection."
    topics = () # TODO

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "coverage": [True, False],
        "formatting": [True, False],
        "examples": [True, False]
    }
    default_options = {"coverage": False, "formatting": True, "examples": True}
    generators = "CMakeToolchain", "CMakeDeps"

    exports_sources = "CMakeLists.txt", "include/*"

    def requirements(self):
        # if self.options.fmt:
        #     self.requires("fmt/10.2.1",
        #                   transitive_headers=True,
        #                   transitive_libs=True)

        self.test_requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)

    def build(self):
        if not self.conf.get("tools.build:skip_test", default=False):
            cmake = CMake(self)
            cmake.configure(
                variables={
                    "ENABLE_COVERAGE": self.options.coverage,
                    "KWARGS_FORMATTING": self.options.formatting,
                    "ENABLE_EXAMPLES": self.options.examples,
                    # "ENABLE_FMTLIB": self.options.fmt,
                }
            )
            cmake.build()

    def package(self):
        copy(self, "*.h", self.source_folder, self.package_folder)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

    def package_id(self):
        self.info.clear()
