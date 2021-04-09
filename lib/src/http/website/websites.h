#ifndef ND_WEBSITE_H_
#define ND_WEBSITE_H_

#include "core.h"

// sites
extern struct WebsiteHandler wxc256;
extern struct WebsiteHandler shuku52vip;
extern struct WebsiteHandler shuku52info;
extern struct WebsiteHandler hnxyrz;

typedef int (*traverse_find_test_func)(xmlNodePtr);

xmlNodePtr traverse_find_first(xmlNodePtr begin, traverse_find_test_func func);

void traverse_find_all(xmlNodePtr begin, traverse_find_test_func func, struct LinkList* list);

int check_tag_name(xmlNodePtr ptr, const char* name);
int check_tag_attr(xmlNodePtr ptr, const char* attr, const char* value);

int check_a(xmlNodePtr);
int check_p(xmlNodePtr);

char* get_node_attr(xmlNodePtr ptr, const char* attr);
char* get_node_text(xmlNodePtr ptr);

void* websiteCreateThreadSharedFunc(int);
void websiteDestroyThreadSharedFunc(void*);

typedef char* (*websiteParsePage)(struct CurlResponse*, struct HttpClient*, struct Chapter*);

void website_do_parallel_work(struct LinkList* urls, websiteParsePage parse);

struct Chapter* allAtoChapters(struct LinkList* link);


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