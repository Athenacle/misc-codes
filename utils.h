#include <spdlog/spdlog.h>

extern "C" struct Novel;

using namespace spdlog::level;

inline level_enum dispatchLevel(int lv)
{
    level_enum lvs[] = {off, err, warn, info, debug, trace};
    if (lv >= 0 && lv <= 5) {
        return lvs[lv];
    } else {
        return spdlog::level::off;
    }
}

void logger_func(int level, const char* msg);


void saveNovel(struct Novel*);