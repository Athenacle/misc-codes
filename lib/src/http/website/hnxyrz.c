#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
//https://www.hnxyrz.com/other/tpl/chapters/id/40725.html

#define WEBSITE_REGEX "^https?://www\\.hnxyrz\\.com/"

#define WEBSITE_NAME "hnxyrz"

static int check(URL url)
{
    return regex_match(url, WEBSITE_REGEX);
}


static int hn_detail_div_con_box(xmlNodePtr ptr)
{
    return check_tag_name(ptr, "div") && check_tag_attr(ptr, "class", "con_box");
}

static int hn_check_parent_div_top(xmlNodePtr ptr)
{
    return check_tag_name(ptr->parent, "div") && check_tag_attr(ptr->parent, "class", "top");
}

static int hn_title_h3(xmlNodePtr ptr)
{
    return hn_check_parent_div_top(ptr) && check_tag_name(ptr, "h3");
}

static int hn_author_p(xmlNodePtr ptr)
{
    return hn_check_parent_div_top(ptr) && check_tag_name(ptr, "p");
}

static void hn_title(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr title = traverse_find_first(head, hn_title_h3);
    if (title) {
        TRACE("body > div.content > div > div.top > h3 found");
        n->title = get_node_text(title);
    }
}

static void hn_author(xmlNodePtr head, struct Novel* n)
{
    xmlNodePtr author = traverse_find_first(head, hn_author_p);
    if (author) {
        TRACE("body > div.content > div > div.top > p found");
        char* a = get_node_text(author);
        if (strstr(a, "作者：") == a) {
            char* start = a + strlen("作者：");
            n->author = strdup(start);
            free(a);
        } else {
            n->author = a;
        }
    }
}

static int hn_class_fL_con(xmlNodePtr node)
{
    return check_tag_name(node, "div") && check_tag_attr(node, "class", "fL_con");
}


static void hn_transform_allA(struct LinkList* node)
{
    void* data = node->data;
    xmlNodePtr ptr = (xmlNodePtr)(data);
    if (ptr) {
        struct Chapter* ch = createChapter();
        ch->url = get_node_attr(ptr, "href");
        ch->title = get_node_text(ptr);
        node->data = ch;
    }
}

static int hn_find_txt(xmlNodePtr ptr)
{
    return check_tag_name(ptr, "div") && check_tag_attr(ptr, "id", "txt");
}

static const char* find(unsigned int in)
{
    uint16_t offset = in - 0xe800;
    static const char pairs[][4] = {
        "的", "一", "是", "了", "我", "不", "人", "在", "他", "有", "这", "个", "上", "们", "来",
        "到", "时", "大", "地", "为", "子", "中", "你", "说", "生", "国", "年", "着", "就", "那",
        "和", "要", "她", "出", "也", "得", "里", "后", "自", "以", "会", "家", "可", "下", "而",
        "过", "天", "去", "能", "对", "小", "多", "然", "于", "心", "学", "么", "之", "都", "好",
        "看", "起", "发", "当", "没", "成", "只", "如", "事", "把", "还", "用", "第", "样", "道",
        "想", "作", "种", "开", "美", "总", "从", "无", "情", "己", "面", "最", "女", "但", "现",
        "前", "些", "所", "同", "日", "手", "又", "行", "意", "动"

    };
    if (offset < sizeof(pairs) / sizeof(pairs[0])) {
        return pairs[offset];
    }
    return NULL;
}

static const char* dispatch(unsigned char* from)
{
    // U-00000800 - U-0000FFFF	-> 1110xxxx 10xxxxxx 10xxxxxx
    unsigned char* p = (unsigned char*)from;

    if (strlen((char*)from) != 3) {
        return (char*)from;
    }

    unsigned char p0 = p[0], p1 = p[1], p2 = p[2], p3 = p[3];

    const char* ret = NULL;
    if (p3 == 0) {
        unsigned char first_high4, first_low4;
        unsigned char second_high2, second_low6;
        unsigned char third_high2, third_low6;

        unsigned int result = 0;

        first_high4 = p0 >> 4;
        first_low4 = p0 & 0xf;

        second_high2 = p1 >> 6;
        second_low6 = p1 & 0x3f;

        third_high2 = p2 >> 6;
        third_low6 = p2 & 0x3f;

        if (first_high4 == 0xe && second_high2 == 0x2 && third_high2 == 0x2) {
            result = first_low4 << 12 | second_low6 << 6 | third_low6;
            ret = find(result);
        }
    }

    if (ret == NULL) {
        char buf[64];
        snprintf(buf, 64, "unknown %x %x %x %x\n", p0, p1, p2, p3);
        ERROR(buf);
        return (char*)from;
    }
    return ret;
}

static void do_save(xmlNodePtr node, struct Buffer* buf)
{
    if (node == NULL) {
        return;
    }
    if (check_tag_name(node, "p")) {
        xmlNodePtr n = node->children;
        appendBufferString(buf, "\n");
        while (n) {
            if (n->type == XML_TEXT_NODE) {
                appendBufferString(buf, (char*)n->content);
            } else if (n->type == XML_ELEMENT_NODE) {
                if (check_tag_name(n, "i")) {
                    appendBufferString(buf, dispatch(n->children->content));
                }
            }
            n = n->next;
        }
    } else {
        if (check_tag_name(node, "i")) {
            appendBufferString(buf, dispatch(node->children->content));
        } else if (node->type == XML_TEXT_NODE) {
            appendBufferString(buf, (char*)node->content);
        }
    }
}

static void hn_content_do_page(xmlNodePtr root, struct Buffer* buf)
{
    xmlNodePtr next = traverse_find_first(root, hn_find_txt);
    if (next) {
        struct LinkList allP;
        xmlNodePtr children = next->children;
        initLinkList(&allP);

        while (children) {
            do_save(children, buf);
            children = children->next;
        }
        freeLinkList(&allP, NULL);
    }
}

static int hn_find_next(xmlNodePtr node)
{
    return check_tag_name(node, "a") && check_tag_attr(node, "class", "url_next");
}

static void hm_check_next_page(xmlNodePtr doc,
                               struct HttpClient* hc,
                               struct Buffer* buf,
                               struct Chapter* c)
{
    xmlNodePtr next = traverse_find_first(doc, hn_find_next);
    if (next) {
        char* href = get_node_attr(next, "href");
        if (strstr(href, "?page=") == href) {
            char* nextUrl = malloc(strlen(c->url) + strlen(href) + 1);
            struct CurlResponse res;
            strcpy(nextUrl, c->url);
            strcat(nextUrl, href);
            client_fetch(nextUrl, hc, &res);

            if (res.status == 200) {
                buildLibXml2(&res);
                if (res.doc) {
                    xmlNodePtr root = xmlDocGetRootElement(res.doc);
                    hn_content_do_page(root, buf);
                    hm_check_next_page(root, hc, buf, c);
                }
            }
            clearCurlResponse(&res);
            free(nextUrl);
        }
        free(href);
    }
}

static char* hn_content_first_page(struct CurlResponse* resp,
                                   struct HttpClient* hc,
                                   struct Chapter* c)
{
    char* ret = NULL;
    xmlNodePtr root = traverse_find_first(xmlDocGetRootElement(resp->doc), hn_find_txt);
    if (root) {
        struct Buffer buf;
        size_t size;
        initBuffer(&buf);

        hn_content_do_page(root, &buf);
        hm_check_next_page(root, hc, &buf, c);

        ret = collectBuffer(&buf, &size);
        clearBuffer(&buf);
    }

    return ret;
}

static void hn_do_download(xmlNodePtr node, struct Novel* n)
{
    xmlNodePtr list = traverse_find_first(node, hn_class_fL_con);
    if (list) {
        struct LinkList allA;
        initLinkList(&allA);
        TRACE("body > div.content > div > div.fenlei > div.fL_con found");

        traverse_find_all(list, check_a, &allA);
        traverseLinkList(&allA, hn_transform_allA);
        website_do_parallel_work(&allA, hn_content_first_page);
        n->chapters = allAtoChapters(&allA);
        freeLinkList(&allA, NULL);
    }
}

static void hn_detail(struct CurlResponse* resp, struct Novel* n)
{
    if (resp->status == 200 && resp->doc != NULL) {
        xmlNodePtr root = xmlDocGetRootElement(resp->doc);
        assert(root != NULL);

        xmlNodePtr head = traverse_find_first(root, hn_detail_div_con_box);
        if (head) {
            TRACE("div.con_box found");
            hn_title(head, n);
            hn_author(head, n);
            hn_do_download(head, n);
        }
    }
}


static void doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (regex_match(url, WEBSITE_REGEX)) {
        INFO(WEBSITE_NAME " novel detail.");
        hn_detail(resp, n);
    }
}


struct WebsiteHandler hnxyrz = {.check = check, .doIt = doit, .name = WEBSITE_NAME};
