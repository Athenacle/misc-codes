#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define WEBSITE_REGEX "^https?://www\\.256wxc\\.com/"

// https://www.256wxc.com/read/60876/
#define WEBSITE_NOVEL_DETAIL_REGEX WEBSITE_REGEX "read/\\d+/?$"

static int check(URL url)
{
    return regex_match(url, WEBSITE_REGEX);
}

static int novel_detail_find_title_node(xmlNodePtr node)
{
    return check_tag_name(node, "h1") && check_tag_attr(node, "class", "art_tit");
}

static int novel_detail_find_author_node(xmlNodePtr node)
{
    return check_tag_name(node, "span") && check_tag_attr(node, "class", "bookinfo")
           && node->children->type == XML_TEXT_NODE;
}

static void novel_detail_title(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr pointer = traverse_find_first(head, novel_detail_find_title_node);
    n->title = get_node_text(pointer);
}

static void novel_detail_author(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr pointer = traverse_find_first(head, novel_detail_find_author_node);
    n->author = get_node_text(pointer);
}

static int novel_detail_find_div_art_head(xmlNodePtr root)
{
    return check_tag_name(root, "div") && check_tag_attr(root, "class", "art_head");
}

static int novel_detail_find_desc_node(xmlNodePtr node)
{
    return check_tag_name(node, "p");
}


static void novel_detail_desc(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr pointer = traverse_find_first(head, novel_detail_find_desc_node);
    n->desc = get_node_text(pointer);
}

static int novel_detail_find_ul_catalog(xmlNodePtr node)
{
    return check_tag_name(node, "ul") && check_tag_attr(node, "class", "catalog");
}

static int novel_detail_find_A_in_catalog(xmlNodePtr node)
{
    return check_tag_name(node, "a") && check_tag_name(node->parent, "li");
}

static void novel_detail_transform_aNODE_url(struct LinkList* node)
{
    void* data = node->data;
    xmlNodePtr ptr = (xmlNodePtr)(data);
    if (ptr) {
        struct Chapter* ch = createChapter();
        ch->url = get_node_attr(ptr, "href");
        node->data = ch;
    }
}

// <div class="book_con fix"
static int novel_detail_find_div_book_con_fix(xmlNodePtr ptr)
{
    return check_tag_name(ptr, "div") && check_tag_attr(ptr, "class", "book_con")
           && check_tag_attr(ptr, "class", "fix");
}

static int novel_detail_find_p(xmlNodePtr ptr)
{
    return check_tag_name(ptr, "p");
}

static void traverseCB_save_content(struct LinkList* list, void* other)
{
    void* data = list->data;
    xmlNodePtr ptr = (xmlNodePtr)data;

    char* result = get_node_text(ptr);
    size_t size = strlen(result);

    (void)list;

    if (size > 0) {
        appendBuffer((struct Buffer*)other, result, size);
        appendBuffer((struct Buffer*)other, "\n", 1);
    }
    free(result);
}

static char* novel_content_page(struct CurlResponse* resp, struct HttpClient* hc, struct Chapter* c)
{
    char* ret = NULL;
    (void)hc;
    (void)c;

    if (resp->doc) {
        xmlNodePtr root = xmlDocGetRootElement(resp->doc);
        xmlNodePtr div = traverse_find_first(root, novel_detail_find_div_book_con_fix);
        if (div) {
            struct LinkList ps;
            struct Buffer buf;
            size_t total = 0;

            initBuffer(&buf);
            initLinkList(&ps);

            traverse_find_all(div, novel_detail_find_p, &ps);
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

    xmlNodePtr catalog = traverse_find_first(root, novel_detail_find_ul_catalog);
    if (catalog) {
        traverse_find_all(catalog, novel_detail_find_A_in_catalog, &allA);
        traverseLinkList(&allA, novel_detail_transform_aNODE_url);


        website_do_parallel_work(&allA, novel_content_page);
        n->chapters = allAtoChapters(&allA);
    }
    freeLinkList(&allA, NULL);
}

static void novel_detail(struct CurlResponse* resp, struct Novel* n)
{
    char buffer[1024];

    xmlNodePtr root = xmlDocGetRootElement(resp->doc);
    assert(root != NULL);

    xmlNodePtr head = traverse_find_first(root, novel_detail_find_div_art_head);
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
    if (regex_match(url, WEBSITE_NOVEL_DETAIL_REGEX)) {
        INFO("WXC256 novel detail.");
        novel_detail(resp, n);
    }
}

struct WebsiteHandler wxc256 = {.check = check, .doIt = doit, .name = "wxc256"};
