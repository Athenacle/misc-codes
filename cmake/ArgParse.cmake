include(ExternalProject)

ExternalProject_Add(
    argparse URL https://github.com/p-ranav/argparse/archive/refs/tags/v2.2.tar.gz
    URL_HASH SHA256=f0fc6ab7e70ac24856c160f44ebb0dd79dc1f7f4a614ee2810d42bb73799872b BUILD_COMMAND
             "" INSTALL_COMMAND "")

ExternalProject_Get_Property(argparse source_dir binary_dir)

set(ARGPARSE_INC_DIR ${source_dir}/include)
