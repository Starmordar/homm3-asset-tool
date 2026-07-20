cmake_minimum_required(VERSION 3.50)

project(sprite-extractor)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(
    sprite_extractor_SRCS
        src/main.cpp
        src/parser/BinaryDataView.cpp
        src/parser/LodDecoder.cpp
        src/parser/DefDecoder.cpp
        src/parser/PcxDecoder.cpp
        src/parser/SpriteWriter.cpp
)

set(
    sprite_extractor_HEADERS
        src/lib/alignment.h
        src/lib/palette.h
        src/lib/file_types.h
        src/parser/FileFormats.h
        src/parser/BinaryDataView.h
        src/parser/LodDecoder.h
        src/parser/DefDecoder.h
        src/parser/PcxDecoder.h
        src/parser/SpriteWriter.h
)
 
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)

add_executable(${PROJECT_NAME} ${sprite_extractor_SRCS})

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(FetchContent)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
)

FetchContent_MakeAvailable(stb)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${stb_SOURCE_DIR}
)

target_link_libraries(sprite-extractor PRIVATE ZLIB::ZLIB)
