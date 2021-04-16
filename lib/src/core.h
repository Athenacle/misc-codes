#ifndef ND_CORE_H_
#define ND_CORE_H_

#include "nd.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>  // for NULL

#include <libxml/HTMLparser.h>

#ifdef HAVE_BZERO
#define SET_ZERO(ptr) bzero(ptr, sizeof(*ptr))
#else
#define SET_ZERO(ptr) memset(ptr, 0, sizeof(*ptr))
#endif

#define STRUCT_MALLOC(type, name) struct type* name = (struct type*)malloc(sizeof(struct type))
#define STRUCT_MALLOC_ZERO(type, name) \
    STRUCT_MALLOC(type, name);         \
    SET_ZERO(name)

// logger
extern ND_logger_func logger;

#define ERROR(msg) logger(NDL_ERROR, (msg))
#define WARN(msg) logger(NDL_WARNING, (msg))
#define INFO(msg) logger(NDL_INFO, (msg))
#define DEBUG(msg) logger(NDL_DEBUG, (msg))
#define TRACE(msg) logger(NDL_TRACE, (msg))

#ifndef NDEBUG

void trace_expresion(const char* ex, const char* file, int line, int value, int should);

#define TRACE_EXPR(expr, should)                                      \
    do {                                                              \
        trace_expresion(#expr, __FILE__, __LINE__, ((expr)), should); \
    } while (0)

#else
#define TRACE_EXPR(expr, should)  \
    do {                          \
        if (should != ((expr))) { \
            WARN(#expr);          \
        }                         \
    } while (0)
#endif

#define CORE_BUFFER_SIZE (1 << 11)

char* getCoreTempBuffer();
void freeCoreTempBuffer(void*);

// types
typedef const char* URL;

struct CurlResponse {
    char* responseHeader;
    size_t htmlLength;
    int status;
    char* html;
    htmlDocPtr doc;
};

struct Buffer {
    char* buffer;
    char* end;
    unsigned int bufferSize;
    struct Buffer* next;
};

struct LinkList {
    struct LinkList* next;
    void* data;
};

struct Queue {
    struct LinkList* head;
    struct LinkList* tail;
};

struct Chapter* createChapter();

void appendChapter(struct Chapter* chapters, struct Chapter* next);

// utils

void initQueue(struct Queue* q);

void* takeQueueFront(struct Queue* q);

void appendQueue(struct Queue* q, void*);


// link list
void initLinkList(struct LinkList* link);

void initLinkListWithCapcity(struct LinkList* link, int cap);

void appendLinkList(struct LinkList* list, void* data);

typedef void (*LinkListTraverser)(struct LinkList*);
void traverseLinkList(struct LinkList* list, LinkListTraverser fn);

typedef void (*LinkListTraverserWithData)(struct LinkList*, void*);
void traverseLinkListWithData(struct LinkList* list, LinkListTraverserWithData fn, void* data);

typedef int (*LinkListSearchFn)(void*, const void*);
void* searchLinkList(struct LinkList* link, LinkListSearchFn fn, const void* data);

void* getLinkListNth(struct LinkList* link, int n);

void freeLinkList(struct LinkList* list, void (*func)(void*));

void clearLinkList(struct LinkList* list, void (*func)(void*));

size_t countLinkListLength(struct LinkList* list);

// buffer
void initBuffer(struct Buffer* buf);

size_t totalSize(struct Buffer* buf);

void appendBuffer(struct Buffer* buf, const void* data, size_t size);

void appendBufferString(struct Buffer* buf, const char* s);

void clearBuffer(struct Buffer* buf);

char* collectBuffer(struct Buffer* buf, size_t* size);

void initCurlResponse(struct CurlResponse* resp);

void clearCurlResponse(struct CurlResponse* resp);

int regex_match(const char* string, const char* regex);

void* regex_compile(const char* regex);
void regex_free(void* regex);
int regex_match_compiled(const char* string, void* regex);

// thread work

struct Work {
    int (*thread_func)(void*, void*);
    void* data;
    int retry;
};

void do_parallel_work(struct LinkList*, void* (*)(int), void (*)(void*));

extern int threadCount;

// http

struct HttpClient {
    void* curl;
};

void fetch(URL url, struct CurlResponse* resp);

void client_init(struct HttpClient* c);

void client_fetch(URL url, struct HttpClient* c, struct CurlResponse* resp);

void client_free(struct HttpClient* c);

// libxml2
void buildLibXml2(struct CurlResponse* resp);


// websites
typedef int (*checkWebsite)(URL);
typedef void (*doNovel)(URL, struct CurlResponse*, struct Novel*);

struct WebsiteHandler {
    checkWebsite check;
    doNovel doIt;
    const char* name;
};

void init_websites();

struct WebsiteHandler* dispatch_url(URL url);

void jjwxc_doit_buffer(void* buffer, unsigned long size, struct JJwxc* j);


#ifndef NDEBUG

void print_func_count(const char* fn, int i, int c);

#define PRINT_FUNC_MISC(c)                \
    do {                                  \
        print_func_count(__func__, 0, c); \
    } while (0)

#define PRINT_FUNC_COUNT                  \
    do {                                  \
        print_func_count(__func__, 1, 0); \
    } while (0)
#else
#define PRINT_FUNC_MISC(c)
#define PRINT_FUNC_COUNT
#endif

#endif
