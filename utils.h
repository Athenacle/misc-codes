
#include <string>

extern "C" struct Novel;

namespace novel
{
    void set_log_level(int);

    void logger_func(int level, const char* msg);

    void saveNovel(struct Novel*);

    struct Flags {
        bool paraSeparate;

        bool paraLeadingSpace;

        int thread;

        std::string url;

        std::string proxy;
    };

    bool parseArgument(Flags&, int argc, const char** argv);

}  // namespace novel