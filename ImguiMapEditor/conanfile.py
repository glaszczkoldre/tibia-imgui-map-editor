from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class TibiaMapEditorConan(ConanFile):
    name = "TibiaMapEditor"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    
    default_options = {
        "glad/*:gl_profile": "compatibility",
        "glad/*:gl_version": "4.6"
    }

    def requirements(self):
        # Graphics & Windowing
        self.requires("glad/0.1.36")
        self.requires("glfw/3.4")
        self.requires("glm/1.0.1")
        
        # Logging & JSON
        self.requires("spdlog/1.16.0")
        self.requires("nlohmann_json/3.12.0")
        
        # XML & File dialogs
        self.requires("pugixml/1.15")
        
        # Compression
        self.requires("lz4/1.10.0")
        self.requires("zlib/1.3.1")
        
        # Boost libraries
        self.requires("boost/1.90.0")
        
        # Image loading
        self.requires("stb/cci.20240531")
        
        # Formatting (dependency of spdlog, but explicit)
        self.requires("fmt/12.0.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
