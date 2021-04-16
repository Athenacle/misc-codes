#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#define WEBSITE_REGEX "^(https?://)www\\.jjwxc\\.net/"
#define WEBSITE_NAME "jjwxc"

static int check(URL url)
{
    return regex_match(url, WEBSITE_REGEX);
}

static int jf_rightul_printright(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "ul") && check_tag_attr(node, "name", "printright");
}

static int jf_r_genre(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && check_tag_attr(node, "itemprop", "genre");
}

static int jf_r_series(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && check_tag_attr(node, "itemprop", "series");
}

static int jf_r_status(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && check_tag_attr(node, "itemprop", "updataStatus");
}

static int jf_r_words(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && check_tag_attr(node, "itemprop", "wordCount");
}

static int jf_span_bigtext(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && check_tag_attr(node, "class", "bigtext");
}

static int jf_span_articleSection(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && check_tag_attr(node, "itemprop", "articleSection");
}

static int jf_tb_oneboolt(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "table") && check_tag_attr(node, "id", "oneboolt");
}

static int jf_span_author(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && check_tag_attr(node, "itemprop", "author");
}

#define JPR(obj, field)                                               \
    do {                                                              \
        const char* ptr = obj->field == NULL ? "empty!" : obj->field; \
        snprintf(buf, 1024, "%s->%s: %s", #obj, #field, ptr);         \
        DEBUG(buf);                                                   \
    } while (0)

#define JPR_N(obj, field)                                            \
    do {                                                             \
        snprintf(buf, 1024, "%s->%s: %d", #obj, #field, obj->field); \
        DEBUG(buf);                                                  \
    } while (0)


static void jj_print(struct JJwxc* jjwxc)
{
    char buf[1024];
    struct Novel* novel = &jjwxc->n;
    JPR(novel, title);
    JPR(novel, author);
    JPR(novel, desc);

    JPR(jjwxc, time);
    JPR(jjwxc, tag);
    JPR(jjwxc, genre);
    JPR(jjwxc, series);
    JPR(jjwxc, updateStatus);
    JPR(jjwxc, look);

    JPR_N(jjwxc, words);
    JPR_N(jjwxc, rid);
    JPR_N(jjwxc, aid);
}

#undef JPR
#undef JPR_N

static int jf_tr_readtd(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "td") && check_tag_attr(node, "class", "readtd");
}

static int jj_novel_extract_word_count(xmlNodePtr node)
{
    if (node == NULL) {
        return 0;
    }

    int ret = 0;
    char* out = get_node_text_raw(node);
    if (out != NULL) {
        char c;
        while ((c = *out) != 0) {
            if (isblank(c) || c == '\n' || c == '\r') {
                out++;
            } else {
                break;
            }
        }
        while ((c = *out++) != 0) {
            if (isdigit(c)) {
                ret = ret * 10 + c - '0';
            } else {
                break;
            }
        }
    }
    return ret;
}

static int fd_div_smallreadbody(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "div") && check_tag_attr(node, "class", "smallreadbody");
}

static void* regex;

static int jf_tag_a(xmlNodePtr node)
{
    assert(regex);
    return check_a(node) && regex_match_compiled(get_node_attr_raw(node, "href"), regex);
}

void jj_extract_tags(struct LinkList* list, void* ptr)
{
    char** data = (char**)ptr;

    xmlNodePtr node = (xmlNodePtr)list->data;
    char* tag = get_node_text_raw(node);
    if (tag) {
        if (*data == NULL) {
            *data = strdup(tag);
        } else {
            char* nb = malloc(strlen(*data) + 2 + strlen(tag));  // 2: one for |, one for \0
            strcpy(nb, *data);
            strcat(nb, "|");
            strcat(nb, tag);
            free(*data);
            *data = nb;
        }
    }
}

static int jf_meta_dataModified(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "meta") && check_tag_attr(node, "itemprop", "dateModified")
           && get_node_attr_raw(node, "content") != NULL;
}

static void jj_updatetime(xmlNodePtr node, struct JJwxc* jj)
{
    xmlNodePtr body = traverse_find_body(node);
    xmlNodePtr dmN = traverse_find_first(body, jf_meta_dataModified);
    jj->time = get_node_attr(dmN, "content");
}

static void jj_novel_full_detail_tag(xmlNodePtr left, struct JJwxc* jj)
{
    struct LinkList list;
    char* tags = NULL;

    xmlNodePtr smallreadbody = traverse_find_first(left, fd_div_smallreadbody);
    initLinkList(&list);
    regex = regex_compile(WEBSITE_REGEX "bookbase\\.php\\?");
    assert(regex);

    traverse_find_all(smallreadbody, jf_tag_a, &list);
    traverseLinkListWithData(&list, jj_extract_tags, &tags);

    freeLinkList(&list, NULL);
    regex_free(regex);
    jj->tag = tags;
}

static int jf_chapter(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "tr") && check_tag_attr(node, "itemprop", "chapter");
}
static int jf_chapter_span_headline(xmlNodePtr node)
{
    return check_span(node) && check_tag_attr(node, "itemprop", "headline");
}
static int jf_chapter_word_count(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "td") && check_tag_attr(node, "itemprop", "wordCount");
}

/*
 *static int jf_chapter_click(xmlNodePtr node)
 *{
 *    return CHECK_TAG_NAME(node, "td") && check_tag_attr(node, "class", "chapterclick");
 *}
 */

static int jf_chapter_time(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "td") && check_tag_attr(node, "title", "章");
}

void jj_chapter_transform(struct LinkList* list)
{
    xmlNodePtr node = (xmlNodePtr)list->data;
    char* lb = getCoreTempBuffer();

    if (node != NULL) {
        STRUCT_MALLOC_ZERO(Chapter, ch);

        xmlNodePtr idN = childFindNext(node->children, check_td);
        xmlNodePtr headN = childFindNext(idN, check_td);
        xmlNodePtr urlN = chainFind(headN, jf_chapter_span_headline, check_a, NULL);

        int id = jj_novel_extract_word_count(idN);
        ch->id = id;

        if (urlN != NULL) {
            xmlNodePtr descN = childFindNext(headN, check_td);
            xmlNodePtr wordN = chainFind(node, jf_chapter_word_count, NULL);
            xmlNodePtr timeN =
                traverse_find_first(traverse_find_child(node, jf_chapter_time), check_span);
            char* url = get_node_attr_raw(urlN, "href");
            int viped = 0;

            if (url != NULL) {
                ch->url = get_node_attr(urlN, "href");
            } else if (NULL != traverse_find_first(urlN->parent, check_font)) {
                viped = 1;
            }
            ch->desc = get_node_text(descN);
            ch->time = get_node_text(timeN);
            ch->title = get_node_text(urlN);
            ch->words = jj_novel_extract_word_count(wordN);

            snprintf(lb,
                     CORE_BUFFER_SIZE,
                     "Chapter #%d(%s): Title: %s, Description: %s, Words: %d, UpdateTime: %s",
                     id,
                     viped ? "VIP" : ch->url,
                     ch->title,
                     ch->desc,
                     ch->words,
                     ch->time);
            DEBUG(lb);

            if (viped) {
                ch->context = strdup(ch->desc);
            }
        } else {
            xmlNodePtr lockN = traverse_find_first(headN, check_font);
            char* reason = get_node_attr_raw(lockN, "title");

            ch->title = strdup("锁");

            if (lockN && reason) {
                snprintf(lb, CORE_BUFFER_SIZE, "Chapter %d Locked: %s", id, reason);
                WARN(lb);

                snprintf(lb, CORE_BUFFER_SIZE, "已锁：%s\n\n", skipBlank((unsigned char*)reason));

                ch->context = strdup(lb);
                ch->desc = strdup(reason);
            }
        }
        list->data = ch;
    } else {
        snprintf(lb, CORE_BUFFER_SIZE, "EMPTY xmlNodePtr found in %s, please check", __func__);
        ERROR(lb);
    }
    freeCoreTempBuffer(lb);
}

static int jj_div_noveltext(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "div") && check_tag_attr(node, "class", "noveltext");
}

static void jj_append_node_text(xmlNodePtr node, struct Buffer* buf)
{
    while (node) {
        if (node->type == XML_TEXT_NODE && node->content != NULL) {
            char* line = (char*)skipBlank(node->content);
            if (*line) {
                appendBufferString(buf, line);
            }
        } else if (CHECK_TAG_NAME(node, "br")) {
            appendBufferString(buf, "\n");
        } else if (CHECK_TAG_NAME(node, "div") && check_tag_attr(node, "class", "readsmall")) {
            appendBufferString(buf, "\n\n**************************************\n");
            jj_append_node_text(node->children, buf);
        }
        node = node->next;
    }
}

static char* jj_do_page(struct CurlResponse* resp,
                        MAYBE_UNUSED struct HttpClient* hc,
                        MAYBE_UNUSED struct Chapter* c)
{
    xmlNodePtr root = xmlDocGetRootElement(resp->doc);
    xmlNodePtr body = traverse_find_body(root);
    xmlNodePtr novelTextN = chainFind(body, jf_tb_oneboolt, jj_div_noveltext, NULL);

    if (novelTextN) {
        xmlNodePtr node = novelTextN->children;
        struct Buffer buf;
        initBuffer(&buf);

        jj_append_node_text(node, &buf);

        appendBufferString(&buf, "\n\n");

        char* ret = collectBuffer(&buf, NULL);
        clearBuffer(&buf);
        return ret;
    }
    return NULL;
}


static void jj_novel_chapter_list(xmlNodePtr node, struct Novel* n)
{
    struct LinkList trs;
    initLinkList(&trs);
    xmlNodePtr table = traverse_find_child(node, jf_tb_oneboolt);
    traverse_find_all(table, jf_chapter, &trs);
    traverseLinkList(&trs, jj_chapter_transform);

    website_do_parallel_work(&trs, jj_do_page);

    n->chapters = allAtoChapters(&trs);
    freeLinkList(&trs, NULL);
}


static void jj_novel_detail(struct CurlResponse* resp, struct Novel* n)
{
    struct LinkList tds;
    xmlNodePtr root = xmlDocGetRootElement(resp->doc);
    if (root == NULL) {
        return;
    }
    initLinkList(&tds);
    xmlNodePtr body = traverse_find_body(root);
    xmlNodePtr titleN = chainFind(body, jf_span_bigtext, jf_span_articleSection, NULL);
    xmlNodePtr authorN = chainFind(body, jf_tb_oneboolt, check_h2, check_a, jf_span_author, NULL);

    n->title = get_node_text(titleN);
    n->author = get_node_text(authorN);

    if (n->desc == NULL) {
        xmlNodePtr descN = traverse_find_nth_child(body, check_table, 1);
        traverse_find_all(descN, jf_tr_readtd, &tds);
        if (countLinkListLength(&tds) == 2) {
            xmlNodePtr first = tds.data;
            n->desc = dumpHTML(first);
        }
    }

    jj_novel_chapter_list(body, n);
    freeLinkList(&tds, NULL);
}

static char* jj_get_series(xmlNodePtr node)
{
    char* ret = NULL;
    if (node == NULL) {
        ret = strdup("");
    } else {
        assert(jf_r_series(node));
        if (get_node_text_raw(node)) {
            ret = get_node_text(node);
        } else {
            xmlNodePtr child = node->children;
            char* lb = getCoreTempBuffer();
            int size = 0;

            while (child) {
                if (child->type == XML_TEXT_NODE && child->content != NULL) {
                    unsigned char* begin = skipBlank(child->content);
                    if (*begin) {
                        int nl = xmlStrlen(begin);
                        if (nl + size >= CORE_BUFFER_SIZE) {
                            break;
                        } else {
                            size += nl;
                            strcat(lb, (char*)begin);
                        }
                    }
                }
                child = child->next;
            }
            if (size > 0) {
                ret = strdup(lb);
            } else {
                ret = strdup("");
            }

            freeCoreTempBuffer(lb);
        }
    }

    return ret;
}

static void jj_novel_full_detail_right(xmlNodePtr right, struct JJwxc* jj)
{
    if (right == NULL) {
        return;
    }
    xmlNodePtr rul = traverse_find_first(right, jf_rightul_printright);
    xmlNodePtr genreN = traverse_find_first(rul, jf_r_genre);
    xmlNodePtr lookN = chainFind(traverse_find_nth_child(rul, check_li, 2), check_span, NULL);
    xmlNodePtr seriesN = traverse_find_first(rul, jf_r_series);
    xmlNodePtr updataStatusN = traverse_find_first(rul, jf_r_status);

    xmlNodePtr finishFontN = NULL;
    if (updataStatusN != NULL) {
        finishFontN = traverse_find_first(updataStatusN->children, check_font);
    }

    xmlNodePtr wcN = traverse_find_first(rul, jf_r_words);

    jj->genre = get_node_text(genreN);
    jj->series = jj_get_series(seriesN);
    jj->words = jj_novel_extract_word_count(wcN);

    if (lookN->next && lookN->next->type == XML_TEXT_NODE) {
        jj->look = get_node_text(lookN->next);
    }
    if (finishFontN) {
        jj->updateStatus = get_node_text(finishFontN);
    } else {
        char* raw = get_node_text_raw(updataStatusN);
        if (raw != NULL && strlen(raw) > 0) {
            jj->updateStatus = get_node_text(updataStatusN);
        }
    }
}


static void jj_id(xmlNodePtr root, struct JJwxc* jj)
{
    xmlNodePtr aidN = findByID(root, "authorid_");
    xmlNodePtr ridN = findByID(root, "clickNovelid");
    jj->aid = jj_novel_extract_word_count(aidN);
    jj->rid = jj_novel_extract_word_count(ridN);
}

static void jj_novel_full_detail(xmlDocPtr doc, struct JJwxc* n)
{
    struct LinkList tds;
    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr body = traverse_find_body(root);
    xmlNodePtr descN = traverse_find_nth_child(body, check_table, 1);

    jj_id(root, n);
    jj_updatetime(root, n);

    initLinkList(&tds);

    traverse_find_all(descN, jf_tr_readtd, &tds);
    if (countLinkListLength(&tds) == 2) {
        xmlNodePtr left = tds.data;
        xmlNodePtr right = tds.next->data;
        n->n.desc = dumpHTML(left);
        jj_novel_full_detail_tag(left, n);
        jj_novel_full_detail_right(right, n);
    }

    freeLinkList(&tds, NULL);
}

void jjwxc_doit_buffer(void* buffer, unsigned long size, struct JJwxc* j)
{
    struct CurlResponse resp;
    SET_ZERO(&resp);
    SET_ZERO(j);

    resp.htmlLength = size;
    resp.status = 200;
    resp.html = malloc(size);
    memcpy(resp.html, buffer, size);
    buildLibXml2(&resp);
    jj_novel_full_detail(resp.doc, j);
    jj_novel_detail(&resp, &j->n);
    jj_print(j);
    clearCurlResponse(&resp);
}

static void jj_doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (regex_match(url, WEBSITE_REGEX)) {
        INFO(WEBSITE_NAME " novel detail.");
        jj_novel_detail(resp, n);
    }
}

void ND_jjwxc_doit(const char* url, struct JJwxc* jj)
{
    struct CurlResponse resp;

    SET_ZERO(jj);

    initCurlResponse(&resp);
    fetch(url, &resp);
    if (resp.status == 200) {
        buildLibXml2(&resp);
        jj_doit(url, &resp, &jj->n);
    }
    clearCurlResponse(&resp);
}

struct WebsiteHandler jjwxc = {.check = check, .doIt = jj_doit, .name = WEBSITE_NAME};
