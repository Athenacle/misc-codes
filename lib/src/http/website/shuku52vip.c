#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define WEBSITE_REGEX "^https?://www\\.52shuku\\.vip/"

static int check(URL url)
{
    return regex_match(url, WEBSITE_REGEX);
}

static int sk52v_title(xmlNodePtr node)
{
    return check_tag_name(node, "h1") && check_tag_attr(node, "class", "article-title");
}

static void sk52v_do_title(struct Novel* n, xmlNodePtr node)
{
    if (node) {
        char* all = get_node_text(node);
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
    return check_tag_name(node, "article") && check_tag_attr(node, "class", "article-content");
}

static void sk52v_desc(xmlNodePtr node, struct Novel* n)
{
    xmlNodePtr ptr = traverse_find_first(node, sk52v_find_desc);

    if (ptr && ptr->children && ptr->children->next && ptr->children->next->next) {
        struct LinkList list;
        initLinkList(&list);

        traverse_find_all(ptr, check_p, &list);

        if (list.next->data) {
            xmlNodePtr p = (xmlNodePtr)list.next->data;
            if (check_p(p)) {
                char* desc = get_node_text(p);
                TRACE(desc);
                n->desc = desc;
            }
        }
        freeLinkList(&list, NULL);
    }
}

static int sk52v_find_mulu(xmlNodePtr node)
{
    return check_tag_name(node, "ul") && check_tag_attr(node, "class", "list");
}

static void transform(struct LinkList* node, void* data)
{
    xmlNodePtr ptr = (xmlNodePtr)(data);
    if (ptr) {
        struct Chapter* ch = createChapter();
        ch->url = get_node_attr(ptr, "href");
        ch->title = get_node_text(ptr);
        node->data = ch;
    }
}

static void extract_content(struct LinkList* node, void* data, void* ud)
{
    (void)node;
    if (data) {
        struct Buffer* buf = (struct Buffer*)ud;
        xmlNodePtr ptr = (xmlNodePtr)data;
        char* c = get_node_text(ptr);
        appendBufferString(buf, c);
        appendBufferString(buf, "\n");
        free(c);
    }
}

static char* sk52v_content_page(struct CurlResponse* resp, struct HttpClient* hc, struct Chapter* c)
{
    (void)hc;
    (void)c;

    xmlNodePtr root = xmlDocGetRootElement(resp->doc);
    assert(root);
    struct LinkList list;
    struct Buffer buf;
    char* ret = NULL;

    xmlNodePtr content = traverse_find_first(root, sk52v_find_desc);
    if (content == NULL) {
        return NULL;
    }

    initLinkList(&list);
    initBuffer(&buf);
    traverse_find_all(content, check_p, &list);
    traverseLinkListWithData(&list, extract_content, &buf);
    ret = collectBuffer(&buf, NULL);
    freeLinkList(&list, NULL);
    clearBuffer(&buf);
    return ret;
}

static int sk52v_a_href(xmlNodePtr ptr)
{
    return check_a(ptr) && check_tag_name(ptr->parent, "li");
}

static void sk52v_mulu(xmlNodePtr node, struct Novel* n)
{
    xmlNodePtr ml = traverse_find_first(node, sk52v_find_mulu);
    if (ml) {
        struct LinkList link;
        initLinkList(&link);
        traverse_find_all(ml, sk52v_a_href, &link);
        traverseLinkList(&link, transform);
        website_do_parallel_work(&link, sk52v_content_page);
        n->chapters = allAtoChapters(&link);
        freeLinkList(&link, NULL);
    }
}

static void sk52v_detail(struct CurlResponse* resp, struct Novel* n)
{
    xmlNodePtr root = xmlDocGetRootElement(resp->doc);
    if (root) {
        sk52v_do_title(n, traverse_find_first(root, sk52v_title));
        sk52v_desc(root, n);

        sk52v_mulu(root, n);
    }
}

static void doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (regex_match(url, WEBSITE_REGEX)) {
        INFO("shuku52vip novel detail.");
        sk52v_detail(resp, n);
    }
}


struct WebsiteHandler shuku52vip = {.check = check, .doIt = doit, .name = "shuku52vip"};
