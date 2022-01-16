#include "websites.h"

#include <string.h>
#include <libxml/HTMLtree.h>

struct WebsiteHandler* handlers[] = {&wxc256, &wenku256, &shuku52vip, &shuku52info, &hnxyrz, &jjwxc};

void initWebsites() {}

int htmlCheck_P(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "p");
}

int htmlCheck_A(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "a");
}

int htmlCheck_H2(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "h2");
}

int htmlCheck_SPAN(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span");
}

int htmlCheck_TD(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "td");
}

int htmlCheck_li(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "li");
}

int htmlCheck_TABLE(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "table");
}

int htmlCheck_FONT(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "font");
}
struct WebsiteHandler* dispatchURL(URL url)
{
    for (size_t i = 0; i < sizeof(handlers) / sizeof(*handlers); i++) {
        struct WebsiteHandler* now = handlers[i];
        if (now->check(url)) {
            char buffer[512];
            snprintf(buffer, 512, "%s found %s for %s", __func__, now->name, url);
            TRACE(buffer);
            return now;
        }
    }
    return NULL;
}


xmlNodePtr htmlFindByID(xmlNodePtr node, const char* id)
{
    PRINT_FUNC_COUNT;

    xmlNodePtr cur_node = NULL;
    xmlNodePtr found = NULL;

    if (NULL == node) {
        return NULL;
    }

    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (htmlCheckNodeAttr(cur_node, "id", id)) {
            found = cur_node;
        } else {
#ifdef NDEBUG
            found = htmlFindByID(cur_node->children, id);
            if (found) {
                return found;
            }
#else
            xmlNodePtr nf = htmlFindByID(cur_node->children, id);
            if (nf) {
                if (found) {
                    char buf[256];
                    snprintf(buf, 256, "duplicated element found for id %s", id);
                    WARN(buf);
                }
                found = nf;
            }
#endif
        }
    }
    return found;
}

static int findBody(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "body");
}

xmlNodePtr htmlChildFindNext(xmlNodePtr begin, htmlFindFunc func)
{
    if (begin == NULL) {
        return NULL;
    }

    do {
        xmlNodePtr next = begin->next;
        if (next != NULL) {
            if (func(next)) {
                return next;
            } else {
                begin = next;
            }
        } else {
            return NULL;
        }

    } while (1);
}
xmlNodePtr htmlChildFindPrev(xmlNodePtr begin, htmlFindFunc func)
{
    if (begin == NULL) {
        return NULL;
    }
    while (begin->prev) {
        if (func(begin->prev)) {
            return begin->prev;
        } else {
            begin = begin->prev;
        }
    }
    return NULL;
}

xmlNodePtr htmlFindBody(xmlNodePtr begin)
{
    return htmlFindFirst(begin, findBody);
}

xmlNodePtr htmlFindNthChild(xmlNodePtr begin, htmlFindFunc func, int n)
{
    PRINT_FUNC_COUNT;

    int i = 0;
    if (begin == NULL) {
        return NULL;
    }

    for (xmlNodePtr ptr = begin->children; ptr; ptr = ptr->next) {
        if (func(ptr)) {
            if (++i == n) {
                return ptr;
            }
        }
    }
    return NULL;
}

xmlNodePtr htmlFindChild(xmlNodePtr begin, htmlFindFunc func)
{
    PRINT_FUNC_COUNT;

    if (begin == NULL) {
        return NULL;
    }

    for (xmlNodePtr n = begin->children; n; n = n->next) {
        if (func(n)) {
            return n;
        }
    }
    return NULL;
}

xmlNodePtr htmlFindFirst(xmlNodePtr begin, htmlFindFunc func)
{
    PRINT_FUNC_COUNT;

    xmlNodePtr cur_node = NULL;
    xmlNodePtr found = NULL;

    if (NULL == begin) {
        return NULL;
    }

    for (cur_node = begin; cur_node; cur_node = cur_node->next) {
        if (func(cur_node)) {
            return cur_node;
        }
        found = htmlFindFirst(cur_node->children, func);
        if (found) {
            return found;
        }
    }
    return NULL;
}

void htmlFindAll(xmlNodePtr begin, htmlFindFunc func, struct LinkList* link)
{
    PRINT_FUNC_COUNT;

    xmlNodePtr cur_node = NULL;

    for (cur_node = begin; cur_node; cur_node = cur_node->next) {
        if (func(cur_node)) {
            appendLinkList(link, cur_node);
        }
        htmlFindAll(cur_node->children, func, link);
    }
}

static xmlAttrPtr hasProp(const xmlNodePtr node, const char* name)
{
    PRINT_FUNC_COUNT;
    xmlAttrPtr prop = NULL;

    if ((node == NULL) || (node->type != XML_ELEMENT_NODE) || (name == NULL))
        return NULL;
    prop = node->properties;
    while (prop != NULL) {
        if (strcmp((const char*)prop->name, name) == 0) {
            break;
        }
        prop = prop->next;
    }
    return prop;
}

int htmlCheckNodeAttr(xmlNodePtr ptr, const char* attr, const char* value)
{
    PRINT_FUNC_COUNT;

    int ret = 0;
    if (ptr) {
        xmlAttrPtr have = hasProp(ptr, attr);
        if (have) {
            ret = strstr((const char*)have->children->content, value) != NULL;
        }
    }
    return ret;
}

char* getNodeTextRaw(xmlNodePtr ptr)
{
    PRINT_FUNC_COUNT;

    if (ptr && ptr->type == XML_ELEMENT_NODE && ptr->children && ptr->children->next == NULL
        && ptr->children->type == XML_TEXT_NODE && ptr->children->content) {
        return ((char*)ptr->children->content);
    }
    return NULL;
}

unsigned char* skipBlank(unsigned char* in)
{
    unsigned char c;
    while ((c = *in) != 0) {
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            in += 1;
            continue;
        } else if (c == 0xe3 && in[1] == 0x80 && in[2] == 0x80) {
            in += 3;
            continue;
        } else {
            break;
        }
    }
    return in;
}

static char* dump_strip_string(const unsigned char* in)
{
    if (in == NULL) {
        return strdup("");
    }
    unsigned char c;
    while ((c = *in++)) {
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            continue;
        } else if (c == 0xe3 && in[0] == 0x80 && in[1] == 0x80) {
            in += 2;
            continue;
        } else {
            in--;
            break;
        }
    }
    char* ret = strdup((const char*)in);
    unsigned char* end = (unsigned char*)ret;
    while (*end++)
        ;
    end -= 1;
    while ((c = *--end)) {
        if ((char*)end == ret) {
            break;
        } else if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            continue;
        } else if (c == 0x80 && *(end - 1) == 0x80 && *(end - 2) == 0xe3) {
            end -= 2;
        } else {
            break;
        }
    }
    *(end + 1) = 0;
    return ret;
}


char* getNodeText(xmlNodePtr ptr)
{
    PRINT_FUNC_COUNT;
    if (ptr) {
        if (ptr->type == XML_ELEMENT_NODE && ptr->children && ptr->children->type == XML_TEXT_NODE
            && ptr->children->content && ptr->children->next == NULL) {
            return dump_strip_string(ptr->children->content);
        } else if (ptr->type == XML_TEXT_NODE && ptr->content) {
            return dump_strip_string(ptr->content);
        }
    }
    return strdup("");
}

char* getNodeAttrRaw(xmlNodePtr ptr, const char* attr)
{
    PRINT_FUNC_COUNT;

    if (ptr) {
        xmlAttrPtr have = hasProp(ptr, attr);
        if (have) {
            return (char*)have->children->content;
        }
    }
    return NULL;
}

char* getNodeAttr(xmlNodePtr ptr, const char* attr)
{
    PRINT_FUNC_COUNT;

    char* r = getNodeAttrRaw(ptr, attr);
    if (r) {
        return strdup(r);
    } else {
        return strdup("");
    }
}


void* websiteCreateThreadSharedFunc(int v)
{
    PRINT_FUNC_COUNT;

    (void)v;
    struct HttpClient* hc = (struct HttpClient*)malloc(sizeof(struct HttpClient));
    initHttpClient(hc);
    return hc;
}

void websiteDestroyThreadSharedFunc(void* s)
{
    PRINT_FUNC_COUNT;

    freeClient((struct HttpClient*)s);
    free(s);
}


struct WebsiteWork {
    websiteParsePage parse;

    struct LinkList* node;
};

static int thread_func(void* ww, void* curl)
{
    struct WebsiteWork* www = (struct WebsiteWork*)ww;
    struct HttpClient* hc = (struct HttpClient*)curl;
    struct CurlResponse resp;
    struct Chapter* chapter = (struct Chapter*)www->node->data;
    char* result = NULL;
    int ret = 0;

    if (chapter && chapter->url && strlen(chapter->url) > 0) {
        fetchClient(chapter->url, hc, &resp);
        if (resp.status == 200 && resp.type == TEXT_HTML) {
            if (resp.data.parser.doc) {
                result = www->parse(&resp, hc, chapter);
                if (result != NULL) {
                    ret = 1;
                    chapter->context = result;
                }
            }
        }
        clearCurlResponse(&resp);
    }
    return ret;
}

static void* buildWork(struct LinkList* url, websiteParsePage parse)
{
    struct Work* ww = (struct Work*)malloc(sizeof(struct Work));
    struct WebsiteWork* www = (struct WebsiteWork*)malloc(sizeof(struct WebsiteWork));
    ww->thread_func = thread_func;
    www->parse = parse;
    www->node = url;
    ww->data = www;
    return ww;
}

struct Chapter* allAtoChapters(struct LinkList* link)
{
    struct Chapter* first = (struct Chapter*)link->data;

    while (link) {
        if (link->next) {
            ((struct Chapter*)link->data)->nextChapter = link->next->data;
        }
        link = link->next;
    }
    return first;
}

void releaseWork(void* d)
{
    free(((struct Work*)d)->data);
    free(d);
}

static void makeWorks(struct LinkList* work, struct LinkList* urls, websiteParsePage parse)
{
    while (urls) {
        appendLinkList(work, buildWork(urls, parse));
        urls = urls->next;
    }
}

void websiteParallelWork(struct LinkList* urls, websiteParsePage parse)
{
    struct LinkList work;
    initLinkList(&work);
    makeWorks(&work, urls, parse);
    doParallelWork(&work, websiteCreateThreadSharedFunc, websiteDestroyThreadSharedFunc);
    freeLinkList(&work, releaseWork);
}

struct Chapter* createChapter(void)
{
    struct Chapter* c = (struct Chapter*)malloc(sizeof(struct Chapter));
    memset(c, 0, sizeof(struct Chapter));
    return c;
}

char* dumpHTML(xmlNodePtr node)
{
    if (node == NULL) {
        return NULL;
    }

    xmlBufferPtr buf = xmlBufferCreate();
    if (htmlNodeDump(buf, node->doc, node) == -1) {
        return NULL;
    }
    char* out = malloc(buf->use + 1);
    memcpy(out, buf->content, buf->use);
    *(out + buf->use) = 0;
    xmlBufferFree(buf);
    return out;
}

xmlNodePtr chainFind(xmlNodePtr begin, ...)
{
    PRINT_FUNC_COUNT;

    va_list args;
    xmlNodePtr b = begin;
    htmlFindFunc fn = NULL;

    va_start(args, begin);
    fn = va_arg(args, htmlFindFunc);
    while (fn && b) {
        b = htmlFindFirst(b, fn);
        if (b == NULL) {
            break;
        }
        fn = va_arg(args, htmlFindFunc);
    }

    va_end(args);
    return b;
}
