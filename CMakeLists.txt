cmake_minimum_required(VERSION 3.50)

project(sprite-extractor)

set(
    sprite_extractor_SRCS
        src/main.cpp
        src/parser/lod_parser.cpp
        src/parser/def_parser.cpp
        src/parser/BinaryDataView.cpp
)

set(
    sprite_extractor_HEADERS
        src/lib/aligment.h
        src/lib/palette.h
        src/lib/def_types.h
        src/parser/lod_parser.h
        src/parser/def_parser.h
        src/parser/BinaryDataView.h
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

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(sprite-extractor PRIVATE ZLIB::ZLIB)
