cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 20)
enable_language(CXX C)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 0)
set(PATCH_VERSION 0)
set(WXAC_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")

set(PRJECT_NAME nDownloader)
project(${PRJECT_NAME} LANGUAGES CXX VERSION ${WXAC_VERSION})

set(OUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/out/)
file(MAKE_DIRECTORY ${OUT_PATH})

set(EXECUTABLE_OUTPUT_PATH ${OUT_PATH})
set(LIBRARY_OUTPUT_PATH ${OUT_PATH})

set(Python_ADDITIONAL_VERSIONS 3.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(THIRD_PARTY_DIR ${CMAKE_BINARY_DIR}/3party)
file(MAKE_DIRECTORY ${THIRD_PARTY_DIR})

include(Check)

add_subdirectory(lib)

add_executable(downloader downloader.cc)
target_link_libraries(downloader ndown)
target_include_directories(downloader PRIVATE lib/include)
