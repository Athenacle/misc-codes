#ifndef ND_WEBSITE_H_
#define ND_WEBSITE_H_

#include "core.h"

// sites
extern struct WebsiteHandler wxc256;

typedef int (*traverse_find_test_func)(xmlNodePtr);

xmlNodePtr traverse_find_first(xmlNodePtr begin, traverse_find_test_func func);

void traverse_find_all(xmlNodePtr begin, traverse_find_test_func func, struct LinkList* list);

int check_tag_name(xmlNodePtr ptr, const char* name);
int check_tag_attr(xmlNodePtr ptr, const char* attr, const char* value);

char* get_node_attr(xmlNodePtr ptr, const char* attr);
char* get_node_text(xmlNodePtr ptr);

void* websiteCreateThreadSharedFunc(int);
void websiteDestroyThreadSharedFunc(void*);

typedef char* (*websiteParsePage)(struct CurlResponse*);

void website_do_parallel_work(struct LinkList* urls, websiteParsePage parse);

struct Chapters {
    struct Chapter* begin;
};

struct Chapter* createChapter(void);
struct Chapters* initChapters(void);
void appendChapters(struct Chapters*, struct Chapter*);

#endif