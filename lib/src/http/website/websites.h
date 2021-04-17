#ifndef ND_WEBSITE_H_
#define ND_WEBSITE_H_

#include "core.h"

// sites
extern struct WebsiteHandler wxc256;
extern struct WebsiteHandler shuku52vip;
extern struct WebsiteHandler shuku52info;
extern struct WebsiteHandler hnxyrz;
extern struct WebsiteHandler jjwxc;

typedef int (*htmlFindFunc)(xmlNodePtr);

xmlNodePtr chainFind(xmlNodePtr begin, ...);

xmlNodePtr htmlFindFirst(xmlNodePtr begin, htmlFindFunc func);
xmlNodePtr htmlFindNthChild(xmlNodePtr begin, htmlFindFunc func, int n);
xmlNodePtr htmlFindBody(xmlNodePtr begin);
xmlNodePtr htmlFindChild(xmlNodePtr begin, htmlFindFunc func);

void htmlFindAll(xmlNodePtr begin, htmlFindFunc func, struct LinkList* list);
xmlNodePtr htmlChildFindNext(xmlNodePtr begin, htmlFindFunc func);
xmlNodePtr htmlChildFindPrev(xmlNodePtr begin, htmlFindFunc func);

xmlNodePtr htmlFindByID(xmlNodePtr node, const char* id);


#define CHECK_TAG_NAME(node, check)                      \
    (((node != NULL) && (node->type == XML_ELEMENT_NODE) \
      && (0 == strcmp((char*)node->name, check))))

int htmlCheckNodeAttr(xmlNodePtr ptr, const char* attr, const char* value);

int htmlCheck_A(xmlNodePtr);
int htmlCheck_P(xmlNodePtr);
int htmlCheck_H2(xmlNodePtr);
int htmlCheck_TABLE(xmlNodePtr);
int htmlCheck_li(xmlNodePtr);
int htmlCheck_FONT(xmlNodePtr);
int htmlCheck_SPAN(xmlNodePtr);
int htmlCheck_TD(xmlNodePtr);

char* getNodeAttr(xmlNodePtr ptr, const char* attr);
char* getNodeAttrRaw(xmlNodePtr ptr, const char* attr);
char* getNodeText(xmlNodePtr ptr);
char* getNodeTextRaw(xmlNodePtr ptr);

void* websiteCreateThreadSharedFunc(int);
void websiteDestroyThreadSharedFunc(void*);

typedef char* (*websiteParsePage)(struct CurlResponse*, struct HttpClient*, struct Chapter*);

void websiteParallelWork(struct LinkList* urls, websiteParsePage parse);

struct Chapter* allAtoChapters(struct LinkList* link);

char* dumpHTML(xmlNodePtr node);

unsigned char* skipBlank(unsigned char* str);

#endif
