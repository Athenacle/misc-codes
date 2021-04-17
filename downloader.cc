
#include "nd.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <cstring>

#include "utils.h"

namespace
{
    [[noreturn]] void usage(const char* argv[])
    {
        auto myname = argv[0];
        std::cerr << myname << ": no url found" << std::endl
                  << "usage: " << myname << " <url>" << std::endl;
        exit(1);
    }
}  // namespace

int main(int argc, const char* argv[])
{
    struct Novel n;

    if (argc != 2) {
        usage(argv);
    }
    novel::set_log_level(NDL_TRACE);
    ND_set_log_function(novel::logger_func);

    ND_init();
    ND_doit(argv[1], &n);
    novel::saveNovel(&n);
    ND_novel_free(&n);
    ND_shutdown();
}
