#ifndef ND_WEBSITE_H_
#define ND_WEBSITE_H_

#include "core.h"

// sites
extern struct WebsiteHandler wxc256;
extern struct WebsiteHandler shuku52vip;
extern struct WebsiteHandler shuku52info;
extern struct WebsiteHandler hnxyrz;
extern struct WebsiteHandler jjwxc;

typedef int (*traverse_find_test_func)(xmlNodePtr);

xmlNodePtr chainFind(xmlNodePtr begin, ...);

xmlNodePtr traverse_find_first(xmlNodePtr begin, traverse_find_test_func func);
xmlNodePtr traverse_find_nth_child(xmlNodePtr begin, traverse_find_test_func func, int n);
xmlNodePtr traverse_find_body(xmlNodePtr begin);
xmlNodePtr traverse_find_child(xmlNodePtr begin, traverse_find_test_func func);

void traverse_find_all(xmlNodePtr begin, traverse_find_test_func func, struct LinkList* list);
xmlNodePtr childFindNext(xmlNodePtr begin, traverse_find_test_func func);
xmlNodePtr childFindPrev(xmlNodePtr begin, traverse_find_test_func func);

xmlNodePtr findByID(xmlNodePtr node, const char* id);


#define CHECK_TAG_NAME(node, check)                      \
    (((node != NULL) && (node->type == XML_ELEMENT_NODE) \
      && (0 == strcmp((char*)node->name, check))))

int check_tag_attr(xmlNodePtr ptr, const char* attr, const char* value);

int check_a(xmlNodePtr);
int check_p(xmlNodePtr);
int check_h2(xmlNodePtr);
int check_table(xmlNodePtr);
int check_li(xmlNodePtr);
int check_font(xmlNodePtr);
int check_span(xmlNodePtr);
int check_td(xmlNodePtr);

char* get_node_attr(xmlNodePtr ptr, const char* attr);
char* get_node_attr_raw(xmlNodePtr ptr, const char* attr);
char* get_node_text(xmlNodePtr ptr);
char* get_node_text_raw(xmlNodePtr ptr);

void* websiteCreateThreadSharedFunc(int);
void websiteDestroyThreadSharedFunc(void*);

typedef char* (*websiteParsePage)(struct CurlResponse*, struct HttpClient*, struct Chapter*);

void website_do_parallel_work(struct LinkList* urls, websiteParsePage parse);

struct Chapter* allAtoChapters(struct LinkList* link);

char* dumpHTML(xmlNodePtr node);

unsigned char* skipBlank(unsigned char* str);

struct Chapters {
    struct Chapter* begin;
};

struct Chapter* createChapter(void);
struct Chapters* initChapters(void);
void appendChapters(struct Chapters*, struct Chapter*);

struct WebsitesHandlerFunctions {
    char* (*to_detail_url)(const char*);
    int (*html_get_root)(xmlNodePtr);
    int (*html_detail_title)(xmlNodePtr);
    int (*html_detail_author)(xmlNodePtr);
    int (*html_detail_desc)(xmlNodePtr);
};

#endif
