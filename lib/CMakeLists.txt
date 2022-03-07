include(FindPkgConfig)

aux_source_directory(src DOWNLOADER_SRC)
aux_source_directory(src/http DOWNLOADER_SRC)
aux_source_directory(src/http/website DOWNLOADER_SRC)

find_package(CURL REQUIRED)
find_package(LibXml2 REQUIRED)
pkg_check_modules(libpcre2-8 REQUIRED libpcre2-8)
pkg_check_modules(opencc REQUIRED opencc)

if (opencc_FOUND)
    set(OPENCC 1)
endif ()

configure_file(${CMAKE_SOURCE_DIR}/lib/src/config.h.in ${CMAKE_SOURCE_DIR}/lib/src/config.h @ONLY)
add_compile_definitions(HAVE_CONFIG_H)

add_library(ndown STATIC ${DOWNLOADER_SRC})
target_include_directories(ndown PRIVATE include/)
target_include_directories(ndown PRIVATE src/)

target_link_libraries(ndown PUBLIC CURL::libcurl LibXml2::LibXml2 $<${OPENCC}:${opencc_LIBRARIES}>
                                   ${libpcre2-8_LIBRARIES} pthread)
