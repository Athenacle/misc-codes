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
    return matchRegex(url, WEBSITE_REGEX);
}

static int jf_rightul_printright(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "ul") && htmlCheckNodeAttr(node, "name", "printright");
}

static int jf_r_genre(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "itemprop", "genre");
}

static int jf_r_series(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "itemprop", "series");
}

static int jf_r_status(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "itemprop", "updataStatus");
}

static int jf_r_words(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "itemprop", "wordCount");
}

static int jf_span_bigtext(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "class", "bigtext");
}

static int jf_span_articleSection(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "itemprop", "articleSection");
}

static int jf_tb_oneboolt(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "table") && htmlCheckNodeAttr(node, "id", "oneboolt");
}

static int jf_span_author(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "span") && htmlCheckNodeAttr(node, "itemprop", "author");
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
    return CHECK_TAG_NAME(node, "td") && htmlCheckNodeAttr(node, "class", "readtd");
}

static int jj_novel_extract_word_count(xmlNodePtr node)
{
    if (node == NULL) {
        return 0;
    }

    int ret = 0;
    char* out = getNodeTextRaw(node);
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
    return CHECK_TAG_NAME(node, "div") && htmlCheckNodeAttr(node, "class", "smallreadbody");
}

static void* regex;

static int jf_tag_a(xmlNodePtr node)
{
    assert(regex);
    return htmlCheck_A(node) && matchRegexCompiled(getNodeAttrRaw(node, "href"), regex);
}

void jj_extract_tags(struct LinkList* list, void* ptr)
{
    char** data = (char**)ptr;

    xmlNodePtr node = (xmlNodePtr)list->data;
    char* tag = getNodeTextRaw(node);
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
    return CHECK_TAG_NAME(node, "meta") && htmlCheckNodeAttr(node, "itemprop", "dateModified")
           && getNodeAttrRaw(node, "content") != NULL;
}

static void jj_updatetime(xmlNodePtr node, struct JJwxc* jj)
{
    xmlNodePtr body = htmlFindBody(node);
    xmlNodePtr dmN = htmlFindFirst(body, jf_meta_dataModified);
    jj->time = getNodeAttr(dmN, "content");
}

static void jj_novel_full_detail_tag(xmlNodePtr left, struct JJwxc* jj)
{
    struct LinkList list;
    char* tags = NULL;

    xmlNodePtr smallreadbody = htmlFindFirst(left, fd_div_smallreadbody);
    initLinkList(&list);
    regex = compileRegex(WEBSITE_REGEX "bookbase\\.php\\?");
    assert(regex);

    htmlFindAll(smallreadbody, jf_tag_a, &list);
    traverseLinkListWithData(&list, jj_extract_tags, &tags);

    freeLinkList(&list, NULL);
    freeRegex(regex);
    jj->tag = tags;
}

static int jf_chapter(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "tr") && htmlCheckNodeAttr(node, "itemprop", "chapter");
}
static int jf_chapter_span_headline(xmlNodePtr node)
{
    return htmlCheck_SPAN(node) && htmlCheckNodeAttr(node, "itemprop", "headline");
}
static int jf_chapter_word_count(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "td") && htmlCheckNodeAttr(node, "itemprop", "wordCount");
}

/*
 *static int jf_chapter_click(xmlNodePtr node)
 *{
 *    return CHECK_TAG_NAME(node, "td") && check_tag_attr(node, "class", "chapterclick");
 *}
 */

static int jf_chapter_time(xmlNodePtr node)
{
    return CHECK_TAG_NAME(node, "td") && htmlCheckNodeAttr(node, "title", "章");
}

void jj_chapter_transform(struct LinkList* list)
{
    xmlNodePtr node = (xmlNodePtr)list->data;
    char* lb = getCoreTempBuffer();

    if (node != NULL) {
        STRUCT_MALLOC_ZERO(Chapter, ch);

        xmlNodePtr idN = htmlChildFindNext(node->children, htmlCheck_TD);
        xmlNodePtr headN = htmlChildFindNext(idN, htmlCheck_TD);
        xmlNodePtr urlN = chainFind(headN, jf_chapter_span_headline, htmlCheck_A, NULL);

        int id = jj_novel_extract_word_count(idN);
        ch->id = id;

        if (urlN != NULL) {
            xmlNodePtr descN = htmlChildFindNext(headN, htmlCheck_TD);
            xmlNodePtr wordN = chainFind(node, jf_chapter_word_count, NULL);
            xmlNodePtr timeN =
                htmlFindFirst(htmlFindChild(node, jf_chapter_time), htmlCheck_SPAN);
            char* url = getNodeAttrRaw(urlN, "href");
            int viped = 0;

            if (url != NULL) {
                ch->url = getNodeAttr(urlN, "href");
            } else if (NULL != htmlFindFirst(urlN->parent, htmlCheck_FONT)) {
                viped = 1;
            }
            ch->desc = getNodeText(descN);
            ch->time = getNodeText(timeN);
            ch->title = getNodeText(urlN);
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
            xmlNodePtr lockN = htmlFindFirst(headN, htmlCheck_FONT);
            char* reason = getNodeAttrRaw(lockN, "title");

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
    return CHECK_TAG_NAME(node, "div") && htmlCheckNodeAttr(node, "class", "noveltext");
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
        } else if (CHECK_TAG_NAME(node, "div") && htmlCheckNodeAttr(node, "class", "readsmall")) {
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
    xmlNodePtr root = xmlDocGetRootElement(resp->data.parser.doc);
    xmlNodePtr body = htmlFindBody(root);
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
    xmlNodePtr table = htmlFindChild(node, jf_tb_oneboolt);
    htmlFindAll(table, jf_chapter, &trs);
    traverseLinkList(&trs, jj_chapter_transform);

    websiteParallelWork(&trs, jj_do_page);

    n->chapters = allAtoChapters(&trs);
    freeLinkList(&trs, NULL);
}


static void jj_novel_detail(struct CurlResponse* resp, struct Novel* n)
{
    struct LinkList tds;
    xmlNodePtr root = xmlDocGetRootElement(resp->data.parser.doc);
    if (root == NULL) {
        return;
    }
    initLinkList(&tds);
    xmlNodePtr body = htmlFindBody(root);
    xmlNodePtr titleN = chainFind(body, jf_span_bigtext, jf_span_articleSection, NULL);
    xmlNodePtr authorN = chainFind(body, jf_tb_oneboolt, htmlCheck_H2, htmlCheck_A, jf_span_author, NULL);

    n->title = getNodeText(titleN);
    n->author = getNodeText(authorN);

    if (n->desc == NULL) {
        xmlNodePtr descN = htmlFindNthChild(body, htmlCheck_TABLE, 1);
        htmlFindAll(descN, jf_tr_readtd, &tds);
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
        if (getNodeTextRaw(node)) {
            ret = getNodeText(node);
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
    xmlNodePtr rul = htmlFindFirst(right, jf_rightul_printright);
    xmlNodePtr genreN = htmlFindFirst(rul, jf_r_genre);
    xmlNodePtr lookN = chainFind(htmlFindNthChild(rul, htmlCheck_li, 2), htmlCheck_SPAN, NULL);
    xmlNodePtr seriesN = htmlFindFirst(rul, jf_r_series);
    xmlNodePtr updataStatusN = htmlFindFirst(rul, jf_r_status);

    xmlNodePtr finishFontN = NULL;
    if (updataStatusN != NULL) {
        finishFontN = htmlFindFirst(updataStatusN->children, htmlCheck_FONT);
    }

    xmlNodePtr wcN = htmlFindFirst(rul, jf_r_words);

    jj->genre = getNodeText(genreN);
    jj->series = jj_get_series(seriesN);
    jj->words = jj_novel_extract_word_count(wcN);

    if (lookN->next && lookN->next->type == XML_TEXT_NODE) {
        jj->look = getNodeText(lookN->next);
    }
    if (finishFontN) {
        jj->updateStatus = getNodeText(finishFontN);
    } else {
        char* raw = getNodeTextRaw(updataStatusN);
        if (raw != NULL && strlen(raw) > 0) {
            jj->updateStatus = getNodeText(updataStatusN);
        }
    }
}


static void jj_id(xmlNodePtr root, struct JJwxc* jj)
{
    xmlNodePtr aidN = htmlFindByID(root, "authorid_");
    xmlNodePtr ridN = htmlFindByID(root, "clickNovelid");
    jj->aid = jj_novel_extract_word_count(aidN);
    jj->rid = jj_novel_extract_word_count(ridN);
}

static void jj_novel_full_detail(xmlDocPtr doc, struct JJwxc* n)
{
    struct LinkList tds;
    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr body = htmlFindBody(root);
    xmlNodePtr descN = htmlFindNthChild(body, htmlCheck_TABLE, 1);

    jj_id(root, n);
    jj_updatetime(root, n);

    initLinkList(&tds);

    htmlFindAll(descN, jf_tr_readtd, &tds);
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

    resp.status = 200;
    resp.type = TEXT_HTML;

    TRACE_EXPR(inputHttpParser(&resp.data.parser, buffer, -1 * size), 0);

    jj_novel_full_detail(resp.data.parser.doc, j);
    jj_novel_detail(&resp, &j->n);
    jj_print(j);
    clearCurlResponse(&resp);
}

static void jj_doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    if (matchRegex(url, WEBSITE_REGEX)) {
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
    if (resp.status == 200 && resp.type == TEXT_HTML) {
        jj_doit(url, &resp, &jj->n);
    }
    clearCurlResponse(&resp);
}

struct WebsiteHandler jjwxc = {.check = check, .doIt = jj_doit, .name = WEBSITE_NAME};
