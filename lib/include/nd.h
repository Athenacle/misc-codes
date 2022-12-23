#ifndef ND_ND_H_
#define ND_ND_H_

#ifdef __cplusplus
extern "C" {
#endif

struct Chapter {
    int id;
    int words;

    const char* time;
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

struct JJwxc {
    struct Novel n;
    int words;
    int rid;
    int aid;

    char* time;
    char* tag;

    char* genre;
    char* series;
    char* updateStatus;
    char* look;
};

struct DownloadConfig {
    int lineSeparate;

    int paraSeparate;

    const char* proxy;
};

char* ND_collect_novel(struct Novel* n);
void ND_free_collected_buffer(char*);

enum LogLevel { NDL_NONE, NDL_ERROR, NDL_WARNING, NDL_INFO, NDL_DEBUG, NDL_TRACE };

typedef void (*ND_logger_func)(int, const char*);

void ND_jjwxc_doit(const char* url, struct JJwxc*);

void ND_jjwxc_doit_buffer(void* buffer, unsigned long size, struct JJwxc*);

void ND_jjwxc_free(struct JJwxc*);

void ND_doit(const char* url, struct Novel* novel);

void ND_init(struct DownloadConfig*);

void ND_shutdown(void);

void ND_set_log_function(ND_logger_func func);

void ND_novel_free(struct Novel* n);

// user-agents
const char* ND_random_ua(void);

// utils
void ND_set_thread_count(int tc);

#ifdef __cplusplus
}
#endif

#endif
