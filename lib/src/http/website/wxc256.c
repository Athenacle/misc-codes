#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define WEBSITE_REGEX "^https?://www\\.256wxc\\.com/"
#define WEBSITE_256WENKU_REGEX "^https?://www\\.256wenku\\.com/"

// https://www.256wxc.com/read/60876/
#define WEBSITE_NOVEL_DETAIL_REGEX WEBSITE_REGEX "read/\\d+/?$"
#define WEBSITE_NOBEL_DETAIL_256WENKU_WEBSITE_REGEX WEBSITE_256WENKU_REGEX "read/\\d+/?$"

static int check_256wenku(URL url)
{
    return matchRegex(url, WEBSITE_256WENKU_REGEX);
}

static int check(URL url)
{
    return matchRegex(url, WEBSITE_REGEX);
}

static int novel_detail_find_title_node(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "h1") && htmlCheckNodeAttr(node, "class", "art_tit");
}

static int novel_detail_find_author_node(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "class", "bookinfo")
           && node->children->type == XML_TEXT_NODE;
}

static void novel_detail_title(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr pointer = htmlFindFirst(head, novel_detail_find_title_node);
    n->title = getNodeText(pointer);
}

static void novel_detail_author(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr pointer = htmlFindFirst(head, novel_detail_find_author_node);
    n->author = getNodeText(pointer);
}

static int novel_detail_find_div_art_head(xmlNodePtr root)
{
    return CHECK_TAG_NAME(root, "div") && htmlCheckNodeAttr(root, "class", "art_head");
}

static int novel_detail_find_desc_node(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "p");
}


static void novel_detail_desc(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr pointer = htmlFindFirst(head, novel_detail_find_desc_node);
    n->desc = getNodeText(pointer);
}

static int novel_detail_find_ul_catalog(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "ul") && htmlCheckNodeAttr(node, "class", "catalog");
}

static int novel_detail_find_A_in_catalog(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "a") && CHECK_TAG_NAME(node->parent, "li");
}

static void novel_detail_transform_aNODE_url(struct LinkList* node)
{
    void* data = node->data;
    xmlNodePtr ptr = (xmlNodePtr)(data);
    if (ptr) {
        struct Chapter* ch = createChapter();
        ch->url = getNodeAttr(ptr, "href");
        node->data = ch;
    }
}

// <div class="book_con fix"
static int novel_detail_find_div_book_con_fix(xmlNodePtr ptr)
{
    return CHECK_TAG_NAME(ptr, "div") && htmlCheckNodeAttr(ptr, "class", "book_con")
           && htmlCheckNodeAttr(ptr, "class", "fix");
}

static int novel_detail_find_p(xmlNodePtr ptr)
{
    return CHECK_TAG_NAME(ptr, "p");
}

static void traverseCB_save_content(struct LinkList* list, void* other)
{
    void* data = list->data;
    xmlNodePtr ptr = (xmlNodePtr)data;

    char* result = getNodeTextRaw(ptr);

    (void)list;

    if (result != NULL) {
        appendBufferString((struct Buffer*)other, result);
        appendBuffer((struct Buffer*)other, "\n", 1);
    }
}

static char* novel_content_page(struct CurlResponse* resp, struct HttpClient* hc, struct Chapter* c)
{
    char* ret = NULL;
    (void)hc;
    (void)c;

    if (resp->data.parser.doc) {
        xmlNodePtr root = xmlDocGetRootElement(resp->data.parser.doc);
        xmlNodePtr div = htmlFindFirst(root, novel_detail_find_div_book_con_fix);
        if (div) {
            struct LinkList ps;
            struct Buffer buf;
            size_t total = 0;

            initBuffer(&buf);
            initLinkList(&ps);

            htmlFindAll(div, novel_detail_find_p, &ps);
            traverseLinkListWithData(&ps, traverseCB_save_content, &buf);

            ret = collectBuffer(&buf, &total);

            freeLinkList(&ps, NULL);
            clearBuffer(&buf);
        }
    }
    return ret;
}

static void novel_detail_get_all_urls(xmlNodePtr root, struct Novel* n)
{
    struct LinkList allA;

    n->chapters = NULL;

    initLinkList(&allA);

    xmlNodePtr catalog = htmlFindFirst(root, novel_detail_find_ul_catalog);
    if (catalog) {
        htmlFindAll(catalog, novel_detail_find_A_in_catalog, &allA);
        traverseLinkList(&allA, novel_detail_transform_aNODE_url);


        websiteParallelWork(&allA, novel_content_page);
        n->chapters = allAtoChapters(&allA);
    }
    freeLinkList(&allA, NULL);
}

static void novel_detail(struct CurlResponse* resp, struct Novel* n)
{
    char buffer[1024];

    xmlNodePtr root = xmlDocGetRootElement(resp->data.parser.doc);
    assert(root != NULL);

    xmlNodePtr head = htmlFindFirst(root, novel_detail_find_div_art_head);
    if (head) {
        novel_detail_title(head, n);
        novel_detail_author(head, n);
        novel_detail_desc(head, n);
    }

    novel_detail_get_all_urls(root, n);
    snprintf(buffer, 1024, "Title %s, Author: %s, Desc: %s", n->title, n->author, n->desc);

    INFO(buffer);
}

static void doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (matchRegex(url, WEBSITE_NOVEL_DETAIL_REGEX)) {
        INFO("WXC256 novel detail.");
        novel_detail(resp, n);
    }
}


static void doit_256wenku(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (matchRegex(url, WEBSITE_NOBEL_DETAIL_256WENKU_WEBSITE_REGEX)) {
        INFO("Wenku256 novel detail.");
        novel_detail(resp, n);
    }
}


struct WebsiteHandler wxc256 = {.check = check, .doIt = doit, .name = "wxc256"};
struct WebsiteHandler wenku256 = {.check = check_256wenku, .doIt = doit_256wenku, .name = "wenku256"};
