
#include "nd.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <cstring>

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

    ND_init();
    ND_doit(argv[1], &n);

    std::string title(n.title);
    title.append(".txt");

    FILE* fp = fopen(title.c_str(), "w");
    if (fp == nullptr) {
        std::cerr << "Open file " << title << " failed: " << strerror(errno) << std::endl;
    } else {
        char* nc = ND_collect_novel(&n);
        if (nc) {
            fprintf(fp, "%s", nc);
            ND_free_collected_buffer(nc);
        }
        fclose(fp);
    }

    ND_clear_novel(&n);
    ND_shutdown();
}
