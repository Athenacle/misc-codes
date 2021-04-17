#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define WEBSITE_REGEX "^https?://www\\.52shuku\\.vip/"

static int check(URL url)
{
    return matchRegex(url, WEBSITE_REGEX);
}

static int sk52v_title(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "h1") && htmlCheckNodeAttr(node, "class", "article-title");
}

static void sk52v_do_title(struct Novel* n, xmlNodePtr node)
{
    if (node) {
        char* all = getNodeText(node);
        if (strlen(all) > 0) {
            char* last = strstr(all, "【");
            char* end = all + strlen(all);

            if (last) {
                *last = 0;
            }
            while (last > all) {
                if (*last == '_') {
                    break;
                }
                last--;
            }
            if (*last == '_' && last < end) {
                *last = 0;
                n->title = strdup(all);
                n->author = strdup(last + 1);
                TRACE(n->title);
                TRACE(n->author);
            }
        }
        free(all);
    }
}

static int sk52v_find_desc(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "article") && htmlCheckNodeAttr(node, "class", "article-content");
}

static void sk52v_desc(xmlNodePtr node, struct Novel* n)
{
    xmlNodePtr ptr = htmlFindFirst(node, sk52v_find_desc);

    if (ptr && ptr->children && ptr->children->next && ptr->children->next->next) {
        struct LinkList list;
        initLinkList(&list);

        htmlFindAll(ptr, htmlCheck_P, &list);

        if (list.next->data) {
            xmlNodePtr p = (xmlNodePtr)list.next->data;
            if (htmlCheck_P(p)) {
                char* desc = getNodeText(p);
                TRACE(desc);
                n->desc = desc;
            }
        }
        freeLinkList(&list, NULL);
    }
}

static int sk52v_find_mulu(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "ul") && htmlCheckNodeAttr(node, "class", "list");
}

static void transform(struct LinkList* node)
{
    void* data = node->data;
    xmlNodePtr ptr = (xmlNodePtr)(data);
    if (ptr) {
        struct Chapter* ch = createChapter();
        ch->url = getNodeAttr(ptr, "href");
        ch->title = getNodeText(ptr);
        node->data = ch;
    }
}

static void sk52v_extract_content(struct LinkList* node, void* ud)
{
    void* data = node->data;
    if (data) {
        struct Buffer* buf = (struct Buffer*)ud;
        xmlNodePtr ptr = (xmlNodePtr)data;
        char* c = getNodeTextRaw(ptr);
        if (c) {
            appendBufferString(buf, c);
            appendBuffer(buf, "\n", 1);
        }
    }
}

static char* sk52v_content_page(struct CurlResponse* resp, struct HttpClient* hc, struct Chapter* c)
{
    (void)hc;
    (void)c;

    xmlNodePtr root = xmlDocGetRootElement(resp->data.parser.doc);
    assert(root);
    struct LinkList list;
    struct Buffer buf;
    char* ret = NULL;

    xmlNodePtr content = htmlFindFirst(root, sk52v_find_desc);
    if (content == NULL) {
        return NULL;
    }

    initLinkList(&list);
    initBuffer(&buf);
    htmlFindAll(content, htmlCheck_P, &list);
    traverseLinkListWithData(&list, sk52v_extract_content, &buf);
    ret = collectBuffer(&buf, NULL);
    freeLinkList(&list, NULL);
    clearBuffer(&buf);
    return ret;
}

static int sk52v_a_href(xmlNodePtr ptr)
{
    return htmlCheck_A(ptr) && CHECK_TAG_NAME(ptr->parent, "li");
}

static void sk52v_mulu(xmlNodePtr node, struct Novel* n)
{
    xmlNodePtr ml = htmlFindFirst(node, sk52v_find_mulu);
    if (ml) {
        struct LinkList link;
        initLinkList(&link);
        htmlFindAll(ml, sk52v_a_href, &link);
        traverseLinkList(&link, transform);
        websiteParallelWork(&link, sk52v_content_page);
        n->chapters = allAtoChapters(&link);
        freeLinkList(&link, NULL);
    }
}

static void sk52v_detail(struct CurlResponse* resp, struct Novel* n)
{
    xmlNodePtr root = xmlDocGetRootElement(resp->data.parser.doc);
    if (root) {
        sk52v_do_title(n, htmlFindFirst(root, sk52v_title));
        sk52v_desc(root, n);

        sk52v_mulu(root, n);
    }
}

static void doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (matchRegex(url, WEBSITE_REGEX)) {
        INFO("shuku52vip novel detail.");
        sk52v_detail(resp, n);
    }
}


struct WebsiteHandler shuku52vip = {.check = check, .doIt = doit, .name = "shuku52vip"};
