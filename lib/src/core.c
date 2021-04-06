#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "http/website/websites.h"

#include <opencc/opencc.h>

#include <curl/curl.h>
#include <pthread.h>

static const char logLevel[][8] = {"[NONE]", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]", "[TRACE]"};


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
    CONVERT(start_url);
    CONVERT(desc);
    doOpenCCChapters(n->chapters);
}

#undef CONVERT

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

void ND_init()
{
    srand(time(NULL));
    init_websites();
    curl_global_init(CURL_GLOBAL_ALL);

    cc = opencc_open(OPENCC_DEFAULT_CONFIG_TRAD_TO_SIMP);

    LIBXML_TEST_VERSION
}

void ND_shutdown()
{
    curl_global_cleanup();
    xmlCleanupParser();
    opencc_close(cc);
}

static void download(const char* url, struct Novel* n)
{
    struct CurlResponse resp;
    struct WebsiteHandler* handler = dispatch_url(url);

    if (handler) {
        initCurlResponse(&resp);
        fetch(url, &resp);
        if (resp.status == 200) {
            buildLibXml2(&resp);
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
    download(url, n);
    doOpenCC(n);
}

void ND_set_log_function(ND_logger_func func)
{
    logger = func;
}

void clearCurlResponse(struct CurlResponse* resp)
{
    if (resp->html) {
        free(resp->html);
    }
    if (resp->doc) {
        xmlFreeDoc(resp->doc);
    }
    if (resp->responseHeader) {
        free(resp->responseHeader);
    }
}

void initCurlResponse(struct CurlResponse* resp)
{
    memset(resp, 0, sizeof(struct CurlResponse));
}

#define HPARSER_OPTION (HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR | HTML_PARSE_RECOVER)

void buildLibXml2(struct CurlResponse* resp)
{
    resp->doc = NULL;
    if (resp->status == 200) {
        htmlParserCtxtPtr ctx = htmlNewParserCtxt();

        resp->doc =
            htmlCtxtReadMemory(ctx, resp->html, resp->htmlLength, NULL, NULL, HPARSER_OPTION);
        htmlFreeParserCtxt(ctx);
        if (xmlDocGetRootElement(resp->doc) == NULL) {
            ERROR("Empty HTML tree.");
            xmlFreeDoc(resp->doc);
            resp->doc = NULL;
        }
    }
}

static void ND_clear_chapter(struct Chapter* n)
{
    struct Chapter* saved = NULL;
    while (n) {
        opencc_convert_utf8_free((char*)n->title);
        opencc_convert_utf8_free((char*)n->desc);
        opencc_convert_utf8_free((char*)n->context);
        free((char*)n->url);
        saved = n;
        n = n->nextChapter;
        free(saved);
    }
}

void ND_clear_novel(struct Novel* n)
{
    opencc_convert_utf8_free((char*)n->author);
    opencc_convert_utf8_free((char*)n->start_url);
    opencc_convert_utf8_free((char*)n->title);
    opencc_convert_utf8_free((char*)n->desc);
    ND_clear_chapter(n->chapters);
}

/*
 * static int countContextLength(struct Chapter* begin)
 * {
 *     int ret = 0;
 *     while (begin) {
 *         ret += strlen(begin->context);
 *         if (begin->title) {
 *             ret += strlen(begin->title);
 *         }
 *         begin = begin->nextChapter;
 *     }
 * }
 */

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
                appendBufferString(&out, "\n");
                appendBufferString(&out, c->title);
                appendBufferString(&out, "\n");
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