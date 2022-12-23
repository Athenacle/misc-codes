include(CheckFunctionExists)
include(CheckIncludeFileCXX)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)
include(CheckSymbolExists)

check_symbol_exists(bzero strings.h HAVE_BZERO)
check_symbol_exists(alloca alloca.h HAVE_ALLOCA)

if (UNIX)
    check_include_file_cxx(pthread.h HAVE_PTHREAD_H)
    check_symbol_exists(getopt_long unistd.h UNIX_HAVE_GETOPTLONG)
    check_symbol_exists(pthread_mutex_lock pthread.h UNIX_HAVE_PTHREAD_MUTEX)
    check_symbol_exists(pthread_barrier_init pthread.h UNIX_HAVE_PTHREAD_BARRIER)
    check_symbol_exists(pthread_cond_init pthread.h UNIX_HAVE_PTHREAD_COND)
    check_symbol_exists(dlopen dlfcn.h UNIX_HAVE_DLOPEN)
    check_symbol_exists(nanosleep time.h UNIX_HAVE_NANOSLEEP)
    check_symbol_exists(get_nprocs sys/sysinfo.h UNIX_HAVE_GET_NPROCS)
    check_symbol_exists(setenv cstdlib UNIX_HAVE_SETENV)
    check_symbol_exists(mmap sys/mman.h UNIX_HAVE_MMAP)
endif ()

check_c_source_compiles(
    "
    #include <pthread.h>
    int main(){
        pthread_spinlock_t *spin;
        pthread_spin_lock(spin);
    }
    "
    UNIX_HAVE_PTHREAD_SPINLOCK)

check_c_source_compiles(
    "
    #include <time.h>
    int main(){return clock_gettime(CLOCK_REALTIME_COARSE, nullptr);}
    "
    HAVE_CLOCK_REALTIME_COARSE)

check_c_source_compiles("int main() {return __builtin_expect(0, 1);}" HAVE_BUILTIN_EXPECT)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    macro (CXX_COMPILER_CHECK_ADD)
        set(list_var "${ARGN}")
        foreach (flag IN LISTS list_var)
            string(TOUPPER ${flag} FLAG_NAME1)
            string(REPLACE "-" "_" FLAG_NAME2 ${FLAG_NAME1})
            string(CONCAT FLAG_NAME "COMPILER_SUPPORT_" ${FLAG_NAME2})
            check_c_compiler_flag(-${flag} ${FLAG_NAME})
            if (${${FLAG_NAME}})
                add_compile_options(-${flag})
            endif ()
        endforeach ()
    endmacro ()

    cxx_compiler_check_add(
        Wall
        #Wno-useless-cast
        Wextra
        Wpedantic
        Wduplicated-branches
        Wduplicated-cond
        Wlogical-op
        Wrestrict
        Wnull-dereference
        Wno-variadic-macros
        #fno-permissive
        )
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        cxx_compiler_check_add(fstack-protector-strong)
    endif ()
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ON_64BITS ON)
endif ()

find_program(GIT_COMMAND git)
execute_process(COMMAND ${GIT_COMMAND} rev-parse HEAD RESULT_VARIABLE result
                OUTPUT_VARIABLE GIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT $"GIT_HASH" STREQUAL "")
    add_compile_definitions(GIT_HASH="${GIT_HASH}")
endif()
