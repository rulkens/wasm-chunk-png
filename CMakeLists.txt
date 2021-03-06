cmake_minimum_required(VERSION 3.7)

set(CMAKE_C_COMPILER "")
set(CMAKE_CXX_COMPILER "")

include($ENV{EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake)

project(chunk_png)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY .tmp/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY .tmp/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY docs)
set(CMAKE_EXECUTABLE_OUTPUT_PATH docs)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_BUILD_TYPE MinSizeRel)
set(CMAKE_C_FLAGS "-O3")
set(CMAKE_CXX_FLAGS "-O3 --std=c++1z")
set(CMAKE_EXE_LINKER_FLAGS "-O3 -g0 --memory-init-file 0 \
-s NO_FILESYSTEM=1 -s DEMANGLE_SUPPORT=0 -s ASSERTIONS=0 -s NO_EXIT_RUNTIME=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s WASM=1 -s DISABLE_EXCEPTION_CATCHING=1 \
-s EXPORTED_FUNCTIONS='[\"_compress\", \"_malloc\", \"_free\"]' \
-s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"getValue\"]'")

include_directories(lib/libpng)
include_directories(lib/zlib)
include_directories($ENV{EMSCRIPTEN}/system/include)

set(OUTPUT_NAME hello)

file(GLOB chunk_png_src "src/main.cpp")
set(sources ${chunk_png_src})

add_executable(${PROJECT_NAME} ${sources})

add_subdirectory(lib/zlib)
add_dependencies(${PROJECT_NAME} zlibstatic)

set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/zlib")
set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/lib/zlib/.tmp/lib/libz.a")
set(M_LIBRARY "")

add_subdirectory(lib/libpng)
add_dependencies(${PROJECT_NAME} png_static)
add_dependencies(png zlibstatic)

target_link_libraries(${PROJECT_NAME}
    zlibstatic
    png
)
