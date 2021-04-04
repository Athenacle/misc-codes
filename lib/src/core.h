#ifndef ND_CORE_H_
#define ND_CORE_H_

#include "nd.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>  // for NULL

#include <libxml/HTMLparser.h>

// logger
extern ND_logger_func logger;

#define ERROR(msg) logger(NDL_ERROR, (msg))
#define WARN(msg) logger(NDL_WARNING, (msg))
#define INFO(msg) logger(NDL_INFO, (msg))
#define DEBUG(msg) logger(NDL_DEBUG, (msg))
#define TRACE(msg) logger(NDL_TRACE, (msg))

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

void initLinkList(struct LinkList* link);

void appendLinkList(struct LinkList* list, void* data);

void traverseLinkList(struct LinkList* list, void (*func)(struct LinkList*, void*));

void traverseLinkListWithData(struct LinkList* list,
                              void (*func)(struct LinkList*, void*, void*),
                              void*);

void freeLinkList(struct LinkList* list, void (*func)(void*));

size_t countLinkListLength(struct LinkList* list);

void initBuffer(struct Buffer* buf);

size_t totalSize(struct Buffer* buf);

void appendBuffer(struct Buffer* buf, const void* data, size_t size);

void appendBufferString(struct Buffer* buf, const char* s);

void clearBuffer(struct Buffer* buf);

char* collectBuffer(struct Buffer* buf, size_t* size);

void initCurlResponse(struct CurlResponse* resp);

void clearCurlResponse(struct CurlResponse* resp);

int regex_match(const char* string, const char* regex);

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

#endif