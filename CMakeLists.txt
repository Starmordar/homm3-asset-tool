cmake_minimum_required(VERSION 3.50)

project(sprite-extractor)

set(
    sprite_extractor_SRCS
        src/main.cpp
        src/parser/parse-lod.cpp
        src/parser/parse-def.cpp
        src/parser/BinaryDataView.cpp
)

set(
    sprite_extractor_HEADERS
        src/parser/file_types.h
        src/parser/parse-lod.h
        src/parser/parse-def.h
        src/parser/BinaryDataView.h
)
 
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)

add_executable(${PROJECT_NAME} ${sprite_extractor_SRCS})

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(sprite-extractor PRIVATE ZLIB::ZLIB)
