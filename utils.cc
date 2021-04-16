#include "nd.h"
#include "utils.h"

#include <iostream>

void logger_func(int level, const char* msg)
{
    auto l = dispatchLevel(level);
    spdlog::log(l, msg);
}

void saveNovel(struct Novel* n)
{
    if (n->title) {
        std::string title(n->title);
        title.append(".txt");

        FILE* fp = fopen(title.c_str(), "w");
        if (fp == nullptr) {
            std::cerr << "Open file " << title << " failed: " << strerror(errno) << std::endl;
        } else {
            char* nc = ND_collect_novel(n);
            if (nc) {
                fprintf(fp, "%s", nc);
                ND_free_collected_buffer(nc);
            }
            fclose(fp);
        }
    }
}