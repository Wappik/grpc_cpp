from conans import ConanFile, CMake, tools

class CategoryProcessorConan(ConanFile):
    name = ""
    license = ""
    author = ""
    topics = ("")
    settings = "os", "compiler", "build_type", "arch"
    requires = (
        "grpc/1.38.0",
        "openssl/1.1.1k", 
        "protobuf/3.17.1",
        "poco/1.10.1@jenkins/stable", 
        "nlohmann_json/3.9.1@jenkins/stable", 
        "oatpp/1.2.0@jenkins/stable", 
    )
    generators = "cmake","cmake_find_package_multi"
    # options = {
    #     "conan_imports": [True, False]
    # }
    default_options = {
        "icu:shared": False,
        "icu:data_packaging": "static",
        "nlohmann_json:multiple_headers": True
        # "conan_imports": True
    }
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto",
    }

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    # def imports(self):
    #     if self.options.conan_imports:
    #         self.copy("*icu*.so*", "deps/icu", "lib")

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

