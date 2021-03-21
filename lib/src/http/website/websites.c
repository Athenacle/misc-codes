#include "websites.h"

#include <string.h>

struct WebsiteHandler* handlers[] = {&wxc256};

void init_websites() {}

static int xml_strcmp(const xmlChar* in, const char* test)
{
    return xmlStrcmp(in, (const xmlChar*)test) == 0;
}

struct WebsiteHandler* dispatch_url(URL url)
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

xmlNodePtr traverse_find_first(xmlNodePtr begin, traverse_find_test_func func)
{
    xmlNodePtr cur_node = NULL;
    xmlNodePtr found = NULL;

    if (NULL == begin) {
        return NULL;
    }

    for (cur_node = begin; cur_node; cur_node = cur_node->next) {
        if (func(cur_node)) {
            return cur_node;
        }
        found = traverse_find_first(cur_node->children, func);
        if (found) {
            return found;
        }
    }
    return NULL;
}

void traverse_find_all(xmlNodePtr begin, traverse_find_test_func func, struct LinkList* link)
{
    xmlNodePtr cur_node = NULL;

    for (cur_node = begin; cur_node; cur_node = cur_node->next) {
        if (func(cur_node)) {
            appendLinkList(link, cur_node);
        }
        traverse_find_all(cur_node->children, func, link);
    }
}

int check_tag_name(xmlNodePtr ptr, const char* name)
{
    return ptr != NULL && ptr->type == XML_ELEMENT_NODE && xml_strcmp(ptr->name, name);
}

int check_tag_attr(xmlNodePtr ptr, const char* attr, const char* value)
{
    int ret = 0;
    if (ptr) {
        xmlChar* v = xmlGetProp(ptr, (const xmlChar*)attr);
        ret = v && xmlStrstr(v, (const xmlChar*)value);
        xmlFree(v);
    }
    return ret;
}

char* get_node_text(xmlNodePtr ptr)
{
    if (ptr && ptr->type == XML_ELEMENT_NODE && ptr->children && ptr->children->next == NULL
        && ptr->children->type == XML_TEXT_NODE && ptr->children->content) {
        return strdup((const char*)ptr->children->content);
    }
    return strdup("");
}

char* get_node_attr(xmlNodePtr ptr, const char* attr)
{
    if (ptr) {
        xmlChar* v = xmlGetProp(ptr, (const xmlChar*)attr);
        if (v != NULL) {
            char* ret = strdup((const char*)v);
            xmlFree(v);
            return ret;
        }
    }
    return strdup("");
}


void* websiteCreateThreadSharedFunc(int v)
{
    (void)v;
    struct HttpClient* hc = (struct HttpClient*)malloc(sizeof(struct HttpClient));
    client_init(hc);
    return hc;
}

void websiteDestroyThreadSharedFunc(void* s)
{
    client_free((struct HttpClient*)s);
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
    char* url = (char*)www->node->data;
    char* result = NULL;
    int ret = 0;
    client_fetch(url, hc, &resp);
    if (resp.status == 200) {
        buildLibXml2(&resp);
        if (resp.doc) {
            result = www->parse(&resp);
            if (result != NULL) {
                ret = 1;
                free(url);
                www->node->data = result;
            }
        }
    }
    clearCurlResponse(&resp);
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

void website_do_parallel_work(struct LinkList* urls, websiteParsePage parse)
{
    struct LinkList work;
    initLinkList(&work);
    makeWorks(&work, urls, parse);
    do_parallel_work(&work, websiteCreateThreadSharedFunc, websiteDestroyThreadSharedFunc);
    freeLinkList(&work, releaseWork);
}