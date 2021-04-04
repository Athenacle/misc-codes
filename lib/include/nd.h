#ifndef ND_ND_H_
#define ND_ND_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Chapter {
    const char* title;
    const char* desc;
    const char* context;
    const char* url;

    struct Chapter* nextChapter;
};

struct Novel {
    const char* title;
    const char* author;
    const char* desc;

    struct Chapter* chapters;
    const char* start_url;
};

char* ND_collect_novel(struct Novel* n);
void ND_free_collected_buffer(char*);

enum LogLevel { NDL_NONE, NDL_ERROR, NDL_WARNING, NDL_INFO, NDL_DEBUG, NDL_TRACE };

typedef void (*ND_logger_func)(int, const char*);

void ND_doit(const char* url, struct Novel* novel);

void ND_init();

void ND_shutdown();

void ND_set_log_function(ND_logger_func func);

void ND_clear_novel(struct Novel* n);

// user-agents
const char* ND_random_ua();

#ifdef __cplusplus
}
#endif

#endif