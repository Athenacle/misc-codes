#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "http/website/websites.h"

#ifdef OPENCC
#include <opencc/opencc.h>
#endif

#include <curl/curl.h>
#include <pthread.h>

static const char logLevel[][8] = {"[NONE]", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]", "[TRACE]"};

#ifdef OPENCC
static opencc_t cc = NULL;

#define CONVERT(field)                                                 \
    do {                                                               \
        if (n->field) {                                                \
            char* p = opencc_convert_utf8(cc, n->field, (size_t)(-1)); \
            free((char*)n->field);                                     \
            n->field = p;                                              \
        }                                                              \
    } while (0)

static void doOpenCCChapters(struct Chapter* n)
{
    while (n) {
        CONVERT(title);
        CONVERT(desc);
        CONVERT(context);
        n = n->nextChapter;
    }
}

static void doOpenCC(struct Novel* n)
{
    CONVERT(title);
    CONVERT(author);
    CONVERT(desc);
    doOpenCCChapters(n->chapters);
}

#undef CONVERT

#endif

#ifndef NDEBUG
struct FuncCount {
    char* name;
    int i;
    int c;
};

struct LinkList funcs;
static pthread_mutex_t* m;

static int search_link_list_func(void* data, const void* in)
{
    return data != NULL && strcmp(((struct FuncCount*)data)->name, (char*)in) == 0;
}
static struct LinkList* findLastDBG(struct LinkList* list)
{
    while (list->next) {
        list = list->next;
    }
    return list;
}

static void appendLinkListDBG(struct LinkList* list, void* data)
{
    if (list->data == NULL) {
        list->data = data;
    } else {
        struct LinkList* last = findLastDBG(list);
        STRUCT_MALLOC_ZERO(LinkList, append);
        append->data = data;
        last->next = append;
    }
}

static void add_func(const char* fn, int i, int c)
{
    void* search = searchLinkList(&funcs, search_link_list_func, fn);
    if (search) {
        ((struct FuncCount*)search)->i++;
    } else {
        STRUCT_MALLOC(FuncCount, n);
        n->name = strdup(fn);

        n->i = n->c = 0;
        n->i += i;
        n->c += c;

        appendLinkListDBG(&funcs, n);
    }
}

void print_func_count(const char* fn, int i, int c)
{
    pthread_mutex_lock(m);
    add_func(fn, i, c);
    pthread_mutex_unlock(m);
}

static void clearFunc(void* fn)
{
    free(((struct FuncCount*)fn)->name);
    free(((struct FuncCount*)fn));
}

void print_func(struct LinkList* l)
{
    char buffer[128];
    struct FuncCount* c = (struct FuncCount*)l->data;
    snprintf(buffer, 128, "%s - %d - %d", c->name, c->i, c->c);
    INFO(buffer);
}

static void clearFuncs(void)
{
    traverseLinkList(&funcs, print_func);
    freeLinkList(&funcs, clearFunc);
    pthread_mutex_destroy(m);
    free(m);
}

static void initFuncs(void)
{
    pthread_mutexattr_t attr;

    m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(m, &attr);
    pthread_mutexattr_destroy(&attr);

    SET_ZERO(&funcs);
}


void trace_expresion(const char* ex, const char* file, int line, int value, int should)
{
    char* lb = getCoreTempBuffer();
    int sz = CORE_BUFFER_SIZE;
    if (value != should) {
        snprintf(lb, sz, "expression %s(%s:%d): %d == %d? failed.", ex, file, line, should, value);
        WARN(lb);
    }
    freeCoreTempBuffer(lb);
}

#endif

static void ND_default_logger(int level, const char* msg)
{
    static pthread_mutex_t consoleMutex = PTHREAD_MUTEX_INITIALIZER;
    const char* lv = "UNKNOWN";
    if (level >= NDL_NONE && level <= NDL_TRACE) {
        lv = logLevel[level];
    }
    pthread_mutex_lock(&consoleMutex);
    fprintf(stderr, "%ld %s: %s\n", time(NULL), lv, msg);
    pthread_mutex_unlock(&consoleMutex);
}

ND_logger_func logger = ND_default_logger;

char* coreBuffer = NULL;
pthread_mutex_t* coreBufferMutex = NULL;

char* getCoreTempBuffer(void)
{
    pthread_mutex_lock(coreBufferMutex);
    *coreBuffer = 0;
    return coreBuffer;
}

void freeCoreTempBuffer(MAYBE_UNUSED void* tb)
{
    assert(tb == coreBuffer);
    pthread_mutex_unlock(coreBufferMutex);
}

static void doAtExit(void)
{
    if (coreBuffer && coreBufferMutex) {
        ND_shutdown();
    }
}

static void skipNewline(char* begin)
{
    char* end = begin;
    while (*end) {
        end++;
    }
    if (*(end - 1) == '\n') {
        *(end - 1) = 0;
    }
}

void xmlErrorPrint(MAYBE_UNUSED void* ctx, const char* msg, ...)
{
    char* buf = getCoreTempBuffer();
    int len = snprintf(buf, CORE_BUFFER_SIZE, "libxml2: %s", msg);
    skipNewline(buf);

    va_list ap;
    va_start(ap, msg);
    vsnprintf(buf + len + 1, CORE_BUFFER_SIZE - len - 1, buf, ap);
    va_end(ap);
    WARN(buf + len + 1);
    freeCoreTempBuffer(buf);
}

static xmlGenericErrorFunc errFunc = xmlErrorPrint;
struct DownloadConfig config;

void ND_init(struct DownloadConfig* c)
{
    memset(&config, 0, sizeof(config));
    if (c != NULL) {
        memcpy(&config, c, sizeof(config));
        if (c->proxy != NULL && strlen(c->proxy) != 0) {
            config.proxy = strdup(c->proxy);
        } else {
            config.proxy = NULL;
        }
    }

    if (coreBufferMutex == NULL) {
#ifndef NDEBUG
        initFuncs();
#endif
        initGenericErrorDefaultFunc(&errFunc);
        xmlSetGenericErrorFunc(NULL, xmlErrorPrint);

        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

        coreBufferMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(coreBufferMutex, &attr);
        pthread_mutexattr_destroy(&attr);

        coreBuffer = (char*)malloc(CORE_BUFFER_SIZE * sizeof(char));
#ifdef GIT_HASH
        snprintf(
            coreBuffer, CORE_BUFFER_SIZE, "ND_init. Version: %s (%s)", PROJECT_VERSION, GIT_HASH);
#else
        snprintf(coreBuffer, CORE_BUFFER_SIZE, "ND_init. Version: %s", PROJECT_VERSION);
#endif
        DEBUG(coreBuffer);

        srand(time(NULL));
        initWebsites();
        curl_global_init(CURL_GLOBAL_ALL);

#ifdef OPENCC
        cc = opencc_open(OPENCC_DEFAULT_CONFIG_TRAD_TO_SIMP);
#endif
        atexit(doAtExit);

        LIBXML_TEST_VERSION
    }
}

void ND_shutdown(void)
{
    curl_global_cleanup();
    xmlCleanupParser();

#ifdef OPENCC
    opencc_close(cc);
#endif

#ifndef NDEBUG
    clearFuncs();
#endif

    pthread_mutex_destroy(coreBufferMutex);
    free(coreBufferMutex);
    free(coreBuffer);
    coreBuffer = NULL;
    coreBufferMutex = NULL;
}

static void download(const char* url, struct Novel* n)
{
    struct CurlResponse resp;
    struct WebsiteHandler* handler = dispatchURL(url);

    if (handler) {
        initCurlResponse(&resp);
        fetch(url, &resp);
        if (resp.status == 200 && resp.type == TEXT_HTML) {
            handler->doIt(url, &resp, n);
        }
        clearCurlResponse(&resp);
    } else {
        char buf[512];
        snprintf(buf, 512, "no Website Handler found for URL %s, exiting...", url);
        ERROR(buf);
    }
}

void ND_doit(const char* url, struct Novel* n)
{
    memset(n, 0, sizeof(struct Novel));
    n->start_url = strdup(url);
    download(url, n);
#ifdef OPENCC
    doOpenCC(n);
#endif
}


void ND_jjwxc_doit_buffer(void* buffer, unsigned long size, struct JJwxc* j)
{
    SET_ZERO(j);
    jjwxc_doit_buffer(buffer, size, j);

#ifdef OPENCC
    doOpenCC(&j->n);
#endif
}

void ND_set_log_function(ND_logger_func func)
{
    logger = func;
}

void clearCurlResponse(struct CurlResponse* resp)
{
    if (resp->type == TEXT_HTML) {
        htmlFreeParserCtxt(resp->data.parser.ctx);
        xmlFreeDoc(resp->data.parser.doc);
        resp->data.parser.ctx = NULL;
        resp->data.parser.doc = NULL;
    } else {
        clearBuffer(&resp->data.buf);
    }
}

void initCurlResponse(struct CurlResponse* resp)
{
    memset(resp, 0, sizeof(struct CurlResponse));
}

static void ND_clear_chapter(struct Chapter* n)
{
    struct Chapter* saved = NULL;

    while (n) {
#ifdef OPENCC
        opencc_convert_utf8_free((char*)n->title);
        opencc_convert_utf8_free((char*)n->desc);
        opencc_convert_utf8_free((char*)n->context);
#else
        free((char*)n->title);
        free((char*)n->desc);
        free((char*)n->context);
#endif
        free((char*)n->url);
        free((char*)n->time);
        saved = n;
        n = n->nextChapter;
        free(saved);
    }
}

void ND_novel_free(struct Novel* n)
{
#ifdef OPENCC
    opencc_convert_utf8_free((char*)n->author);
    opencc_convert_utf8_free((char*)n->title);
    opencc_convert_utf8_free((char*)n->desc);
#else
    free((char*)n->author);
    free((char*)n->title);
    free((char*)n->desc);
#endif
    ND_clear_chapter(n->chapters);
    free((char*)n->start_url);
}

char* ND_collect_novel(struct Novel* n)
{
    char* ret = NULL;
    struct Chapter* c = n->chapters;
    if (n->chapters) {
        struct Buffer out;
        size_t size;

        initBuffer(&out);
        while (c) {
            if (c->title) {
                if (c->id > 0) {
                    char nbuf[64];
                    snprintf(nbuf, 64, "\n第%d章   ", c->id);
                    appendBufferString(&out, nbuf);
                    appendBufferString(&out, c->title);
                    appendBufferString(&out, "\n");
                } else {
                    appendBufferString(&out, "\n");
                    appendBufferString(&out, c->title);
                    appendBufferString(&out, "\n");
                }
            }
            appendBufferString(&out, c->context);
            c = c->nextChapter;
        }
        ret = collectBuffer(&out, &size);
        clearBuffer(&out);
    }
    return ret;
}
void ND_free_collected_buffer(char* b)
{
    free(b);
}

void ND_jjwxc_free(struct JJwxc* jj)
{
    ND_novel_free(&jj->n);
    free(jj->time);
    free(jj->tag);
    free(jj->genre);
    free(jj->series);
    free(jj->updateStatus);
    free(jj->look);
}

#ifdef UNIX
#include <unistd.h>
int sleepSeconds(int s)
{
    return sleep((unsigned int)s);
}
#endif
