#include "nd.h"
#include "utils.h"

#include <spdlog/spdlog.h>

#include <iostream>

using namespace spdlog::level;

namespace
{
    inline level_enum dispatchLevel(int lv)
    {
        level_enum lvs[] = {off, err, warn, info, debug, trace};
        if (lv >= 0 && lv <= 5) {
            return lvs[lv];
        } else {
            return spdlog::level::off;
        }
    }
}  // namespace

namespace novel
{
    void set_log_level(int lv)
    {
        spdlog::set_level(dispatchLevel(lv));
    }


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
}  // namespace novel
