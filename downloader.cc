
#include "nd.h"

#include <cstdio>
#include <string>
#include <iostream>
#include <cstring>

#include "utils.h"


int main(int argc, const char* argv[])
{
    struct Novel n;
    novel::Flags f;
    DownloadConfig c;

    parseArgument(f, argc, argv);
    c.lineSeparate = f.paraLeadingSpace;
    c.paraSeparate = f.paraSeparate;
    c.proxy = f.proxy.c_str();

    novel::set_log_level(NDL_TRACE);
    ND_set_log_function(novel::logger_func);

    ND_init(&c);
    ND_doit(f.url.c_str(), &n);
    novel::saveNovel(&n);
    ND_novel_free(&n);
    ND_shutdown();
}
