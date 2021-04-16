#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define WEBSITE_REGEX "^https?://www\\.52shuku\\.info/"
#define WEBSITE_NAME "shuku52.info"

static int check(URL url)
{
    return regex_match(url, WEBSITE_REGEX);
}

static int sk52i_title(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "h1") && check_tag_attr(node, "class", "booktitle");
}

static int sk52i_author(xmlNodePtr node)
{
    return check_a(node) && CHECK_TAG_NAME(node->parent, "p")
           && check_tag_attr(node, "class", "red")
           && check_tag_attr(node->parent, "class", "booktag");
}

static int sk52i_intro(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "p") && check_tag_attr(node, "class", "bookintro")
           && check_tag_attr(node->parent, "class", "bookinfo");
}

static void sk52i_do_title(xmlNodePtr node, struct Novel* n)
{
    n->title = get_node_text(traverse_find_first(node, sk52i_title));
    n->author = get_node_text(traverse_find_first(node, sk52i_author));
    n->desc = get_node_text(traverse_find_first(node, sk52i_intro));
}

static void sk52i_extract_content(struct LinkList* node, void* ud)
{
    void* data = node->data;
    if (data) {
        struct Buffer* buf = (struct Buffer*)ud;
        xmlNodePtr ptr = (xmlNodePtr)data;
        assert(ptr->type == XML_TEXT_NODE);
        appendBufferString(buf, (char*)ptr->content);
        appendBufferString(buf, "\n");
    }
}

static int sk52i_content_text(xmlNodePtr node)
{
    return node->type == XML_TEXT_NODE && CHECK_TAG_NAME(node->parent, "div")
           && check_tag_attr(node->parent, "class", "readcontent");
}

static int sk52i_find_next(xmlNodePtr node)
{
    return check_p(node) && check_tag_attr(node, "class", "text-danger")
           && check_tag_attr(node, "class", "text-center");
}

static int sk52i_next_a_href(xmlNodePtr node)
{
    return check_a(node) && check_tag_attr(node, "id", "linkNext");
}

#define WEBSITE_BASE_URL "https://www.52shuku.info/"

static void sk52i_check_next(xmlNodePtr node,
                             struct Chapter* c,
                             struct HttpClient* hc,
                             struct Buffer* buf)
{
    xmlNodePtr next = traverse_find_first(node, sk52i_find_next);
    xmlNodePtr na = traverse_find_first(node, sk52i_next_a_href);
    if (next && na) {
        struct LinkList list;
        struct CurlResponse resp;
        char* href = get_node_attr(na, "href");
        char* url = (char*)malloc(sizeof(WEBSITE_BASE_URL) + strlen(href) + 1);
        strcpy(url, WEBSITE_BASE_URL);
        strcat(url, href);

        initLinkList(&list);

        client_fetch(url, hc, &resp);
        if (resp.status == 200) {
            buildLibXml2(&resp);
            if (resp.doc) {
                xmlNodePtr root = xmlDocGetRootElement(resp.doc);
                traverse_find_all(root, sk52i_content_text, &list);
                traverseLinkListWithData(&list, sk52i_extract_content, buf);
                sk52i_check_next(root, c, hc, buf);
            }
        }
        clearCurlResponse(&resp);
        free(href);
        free(url);
        freeLinkList(&list, NULL);
    }
}

static char* sk52i_content_page(struct CurlResponse* resp, struct HttpClient* hc, struct Chapter* c)
{
    (void)hc;
    (void)c;

    xmlNodePtr root = xmlDocGetRootElement(resp->doc);
    assert(root);
    struct LinkList list;
    struct Buffer buf;
    char* ret = NULL;

    xmlNodePtr content = traverse_find_first(root, sk52i_content_text);
    if (content == NULL) {
        return NULL;
    }

    initLinkList(&list);
    initBuffer(&buf);

    traverse_find_all(content, sk52i_content_text, &list);
    traverseLinkListWithData(&list, sk52i_extract_content, &buf);

    sk52i_check_next(root, c, hc, &buf);


    ret = collectBuffer(&buf, NULL);
    freeLinkList(&list, NULL);
    clearBuffer(&buf);
    return ret;
}

static void sk52i_transform(struct LinkList* node, void* n)
{
    void* data = node->data;
    xmlNodePtr ptr = (xmlNodePtr)(data);
    struct Novel* np = (struct Novel*)n;
    if (ptr) {
        struct Chapter* ch = createChapter();
        char* h = get_node_attr(ptr, "href");
        char* u = (char*)malloc(strlen(np->start_url) + strlen(h) + 1);
        strcpy(u, np->start_url);
        strcat(u, h);
        ch->title = get_node_text(ptr);
        ch->url = u;
        node->data = ch;
        free(h);
    }
}


static int sk52i_a(xmlNodePtr node)
{
    return check_a(node) && CHECK_TAG_NAME(node->parent, "dd");
}

static void sk52i_context(xmlNodePtr root, struct Novel* n)
{
    struct LinkList list;
    initLinkList(&list);
    traverse_find_all(root, sk52i_a, &list);
    traverseLinkListWithData(&list, sk52i_transform, n);
    website_do_parallel_work(&list, sk52i_content_page);
    n->chapters = allAtoChapters(&list);
    freeLinkList(&list, NULL);
}

static void sk52i_detail(struct CurlResponse* resp, struct Novel* n)
{
    xmlNodePtr root = xmlDocGetRootElement(resp->doc);
    if (root) {
        sk52i_do_title(root, n);
        sk52i_context(root, n);
    }
}

static void doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (regex_match(url, WEBSITE_REGEX)) {
        INFO(WEBSITE_NAME " novel detail.");
        sk52i_detail(resp, n);
    }
}


struct WebsiteHandler shuku52info = {.check = check, .doIt = doit, .name = WEBSITE_NAME};
