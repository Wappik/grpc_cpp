cmake_minimum_required(VERSION 3.16)
project(client)


# set(CONAN_DISABLE_CHECK_COMPILER TRUE)
if (CONAN_EXPORTED) # in conan local cache
    # standard conan installation, deps will be defined in conanfile.py
    # and not necessary to call conan again, conan is already running
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
else ()
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

    message(STATUS "CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH}")
    message(STATUS "CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}")

    include(conan/conan.cmake)
    conan_cmake_run(
            CONANFILE conan/conanfile.py
            BASIC_SETUP CMAKE_TARGETS KEEP_RPATHS
            PROFILE default
            PROFILE_AUTO ALL
            BUILD missing)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

set(CMAKE_CXX_STANDARD 17)

find_package(protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

message(STATUS "protoc: ${_PROTOBUF_PROTOC}")
message(STATUS "grpc_cpp_plugin: ${_GRPC_CPP_PLUGIN_EXECUTABLE}")

get_filename_component(copy_service_proto "protos/copy_service.proto" ABSOLUTE)
get_filename_component(copy_service_proto_path "${copy_service_proto}" PATH)

set(copy_service_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/copy_service.pb.cc")
set(copy_service_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/copy_service.pb.h")
set(copy_service_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/copy_service.grpc.pb.cc")
set(copy_service_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/copy_service.grpc.pb.h")

add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-array-bounds
        -Wdeprecated-declarations
        -Wno-unused-parameter
        -Wno-unused-variable
        -fno-omit-frame-pointer
        -Wno-reorder
)

add_custom_command(
        OUTPUT "${copy_service_proto_srcs}" "${copy_service_proto_hdrs}" "${copy_service_grpc_srcs}" "${copy_service_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${copy_service_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${copy_service_proto}"
        DEPENDS "${copy_service_proto}"
        )

add_executable(client
        ${copy_service_proto_srcs} ${copy_service_grpc_srcs}
        include/ClientInterface.hpp
        src/ClientImpl.cpp
        include/ClientImpl.hpp
        src/RpcHandler.cpp
        include/RpcHandler.hpp
        src/Client.cpp
        include/Client.hpp
        include/Models.hpp
        src/ClientService.cpp
        include/ClientService.hpp
        src/ClientInterface.cpp
        src/ConnectionController.cpp
        include/ConnectionController.hpp
)

target_include_directories(client PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}"
    include
    )

target_link_libraries(client PUBLIC
        CONAN_PKG::poco
        CONAN_PKG::nlohmann_json
        CONAN_PKG::grpc
        CONAN_PKG::oatpp
        CONAN_PKG::protobuf
    )