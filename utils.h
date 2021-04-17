
extern "C" struct Novel;

namespace novel
{
    void set_log_level(int);

    void logger_func(int level, const char* msg);

    void saveNovel(struct Novel*);
}  // namespace novel