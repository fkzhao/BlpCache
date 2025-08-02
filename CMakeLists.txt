cmake_minimum_required(VERSION 3.31)
project(BlpCache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

add_compile_options(-Wno-deprecated-declarations)
include_directories(/usr/local/include)
if(APPLE)
    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)
endif ()
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party")
find_library(BRPC_LIB NAMES brpc)
find_library(PROTOBUF_LIB NAMES protobuf)
find_library(LEVELDB_LIB NAMES leveldb)
find_library(FMT_LIB NAMES fmt)
file(GLOB ABSEIL_LIBS "/opt/homebrew/lib/libabsl_*.dylib")



set(DYNAMIC_LIB
        ${BRPC_LIB}
        ${PROTOBUF_LIB}
        ${LEVELDB_LIB}
        ${FMT_LIB}
        ${ABSEIL_LIBS}
        pthread
)

file(GLOB SOURCE_FILES
        "src/common/*.h"
        "src/common/*.cpp"
        "src/common/*.cc"
        "src/core/*.h"
        "src/core/*.cpp"
        "src/core/*.cc"
        "src/service/*.h"
        "src/service/*.cpp"
        "src/service/*.cc"
        "src/replcate/*.h"
        "src/replcate/*.cpp"
        "src/replcate/*.cc"
)
add_executable(BlpCache ${SOURCE_FILES} src/blp.cpp)
set_target_properties(BlpCache PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
target_link_libraries(BlpCache PRIVATE ${DYNAMIC_LIB})


add_executable(SlaveDemo ${SOURCE_FILES} src/slave.cpp)
set_target_properties(SlaveDemo PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
target_link_libraries(SlaveDemo PRIVATE ${DYNAMIC_LIB})