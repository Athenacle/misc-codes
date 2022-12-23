#include "core.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <pthread.h>

#define DEFAULT_BUFFER_BLOCK_SIZE (1 << 14)

struct Buffer* createBuffer(size_t size)
{
    PRINT_FUNC_COUNT;
    PRINT_FUNC_MISC(size);

    struct Buffer* ret = (struct Buffer*)malloc(sizeof(struct Buffer));
    if (size > DEFAULT_BUFFER_BLOCK_SIZE) {
        ret->bufferSize = size;
        ret->buffer = ret->end = malloc(size);
        ret->next = NULL;
    } else {
        initBuffer(ret);
    }

    return ret;
}

void initBuffer(struct Buffer* buf)
{
    PRINT_FUNC_COUNT;

    buf->bufferSize = DEFAULT_BUFFER_BLOCK_SIZE;
    buf->end = buf->buffer = malloc(buf->bufferSize);
    buf->next = NULL;
}

size_t totalSize(struct Buffer* buf)
{
    size_t total = 0;
    struct Buffer* next = buf;

    while (next) {
        total += (next->end - next->buffer);
        next = next->next;
    }

    return total;
}

void appendBufferString(struct Buffer* buf, const char* s)
{
    if (s == NULL) {
        return;
    }
    appendBuffer(buf, s, strlen(s));
}

void appendBuffer(struct Buffer* buf, const void* data, size_t size)
{
    PRINT_FUNC_COUNT;
    PRINT_FUNC_MISC(size);

    struct Buffer* last = buf;
    size_t remain;

    if (data == NULL || size == 0) {
        return;
    }

    while (last->next) {
        last = last->next;
    }

    remain = last->bufferSize - (last->end - last->buffer);

    if (remain < size) {
        memcpy(last->end, data, remain);
        last->end += remain;
        last->next = createBuffer(size - remain);

        memcpy(last->next->buffer, (const char*)data + (remain), size - remain);
        last->next->end = (last->next->buffer + size - remain);
    } else {
        memcpy(last->end, data, size);
        last->end += size;
    }
}

char* collectBuffer(struct Buffer* buf, size_t* size)
{
    PRINT_FUNC_COUNT;

    size_t s = totalSize(buf);
    char* ret = (char*)malloc(s + 1);
    char* ptr = ret;
    struct Buffer* next = buf;
    if (size != NULL) {
        *size = s;
    }

    while (next) {
        size_t count = next->end - next->buffer;
        memcpy(ptr, next->buffer, count);
        next = next->next;
        ptr += count;
    }
    ret[s] = 0;

    return ret;
}


void clearBuffer(struct Buffer* buf)
{
    PRINT_FUNC_COUNT;

    struct Buffer* next = buf->next;
    while (next) {
        struct Buffer* nn = next->next;
        free(next->buffer);
        free(next);
        next = nn;
    }
    free(buf->buffer);
    buf->buffer = buf->end = NULL;
    buf->next = NULL;
}


void* compileRegex(const char* regex)
{
    int errornumber;
    PCRE2_SIZE erroroffset;

    return pcre2_compile(
        (PCRE2_SPTR)regex, PCRE2_ZERO_TERMINATED, 0, &errornumber, &erroroffset, NULL);
}

void freeRegex(void* regex)
{
    pcre2_code_free(regex);
}

int matchRegexCompiled(const char* string, void* regex)
{
    if (regex == NULL) {
        return 0;
    }
    int rc = 0;
    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    rc = pcre2_match(regex, (PCRE2_SPTR)string, strlen(string), 0, 0, match_data, NULL);
    pcre2_match_data_free(match_data);
    return rc > 0;
}

int matchRegex(const char* string, const char* regex)
{
    pcre2_code* re = compileRegex(regex);
    int rc = matchRegexCompiled(string, re);
    freeRegex(re);
    return rc;
}

void initLinkList(struct LinkList* link)
{
    PRINT_FUNC_COUNT;

    link->data = NULL;
    link->next = NULL;
}

void* searchLinkList(struct LinkList* link, LinkListSearchFn fn, const void* data)
{
    while (link) {
        if (fn(link->data, data)) {
            return link->data;
        }
        link = link->next;
    }
    return NULL;
}

void initLinkListWithCapcity(struct LinkList* link, int cap)
{
    if (cap > 1 && cap < 100) {
        struct LinkList* block = (struct LinkList*)malloc((cap - 1) * sizeof(struct LinkList));
        link->next = block;
    } else {
        link->next = NULL;
    }
    link->data = NULL;
}

static struct LinkList* findLast(struct LinkList* list)
{
    PRINT_FUNC_COUNT;

    while (1) {
        if (list->data == NULL) {
            return list;
        } else {
            if (list->next == NULL) {
                return list;
            } else {
                list = list->next;
            }
        }
    }
    return list;
}

static struct LinkList* createLinkNode(void* data)
{
    PRINT_FUNC_COUNT;

    struct LinkList* append = (struct LinkList*)malloc(sizeof(struct LinkList));
    append->data = data;
    append->next = NULL;
    return append;
}

void traverseLinkListWithData(struct LinkList* list, LinkListTraverserWithData fn, void* data)
{
    PRINT_FUNC_COUNT;

    while (list) {
        fn(list, data);
        list = list->next;
    }
}


void traverseLinkList(struct LinkList* list, LinkListTraverser fn)
{
    PRINT_FUNC_COUNT;

    while (list) {
        fn(list);
        list = list->next;
    }
}

void appendLinkList(struct LinkList* list, void* data)
{
    PRINT_FUNC_COUNT;

    if (list->data == NULL) {
        list->data = data;
    } else {
        struct LinkList* last = findLast(list);
        if (last->data == NULL) {
            last->data = data;
        } else {
            last->next = createLinkNode(data);
        }
    }
}

void freeLinkList(struct LinkList* list, void (*free_func)(void*))
{
    PRINT_FUNC_COUNT;

    struct LinkList *next, *tmp;

    if (list == NULL) {
        return;
    }
    next = list->next;
    while (next) {
        if (free_func) {
            free_func(next->data);
        }
        tmp = next->next;
        free(next);
        next = tmp;
    }
    if (free_func) {
        free_func(list->data);
    }
    list->data = list->next = NULL;
}

void clearLinkList(struct LinkList* list, void (*func)(void*))
{
    PRINT_FUNC_COUNT;

    while (list) {
        if (list->data && func) {
            func(list->data);
        }
        list->data = NULL;
        list = list->next;
    }
}

void* getLinkListNth(struct LinkList* list, int n)
{
    if (n < 0) {
        return NULL;
    }
    while (n > 0 && list != NULL) {
        list = list->next;
        n--;
    }

    return list ? list->data : NULL;
}

size_t countLinkListLength(struct LinkList* list)
{
    size_t ret = 0;
    while (list) {
        ret += 1;
        list = list->next;
    }
    if (ret > 0) {
        ret -= 1;
    }
    return ret;
}

void initQueue(struct Queue* q)
{
    q->head = q->tail = NULL;
}

void appendQueue(struct Queue* q, void* data)
{
    if (q->head == NULL) {
        assert(q->head == q->tail);
        q->tail = q->head = createLinkNode(data);
    } else {
        assert(q->tail != NULL && q->tail->next == NULL);
        q->tail->next = createLinkNode(data);
        q->tail = q->tail->next;
    }
}

void* takeQueueFront(struct Queue* q)
{
    void* ret;
    struct LinkList* now = q->head;
    if (now == NULL) {
        assert(q->head == q->tail);
        return NULL;
    }
    ret = now->data;
    q->head = now->next;
    if (q->head == NULL) {
        assert(q->tail == now);
        q->tail = NULL;
    }
    free(now);
    return ret;
}

int threadCount = 6;

struct ParallelWork {
    struct Queue queue;
    pthread_mutex_t mutex;

    void* (*tsCreate)(int);
    void (*tsFree)(void*);
};


static struct Work* takeWork(struct ParallelWork* works)
{
    void* take;
    pthread_mutex_lock(&works->mutex);
    take = takeQueueFront(&works->queue);
    pthread_mutex_unlock(&works->mutex);
    return (struct Work*)take;
}

static void appendWork(struct ParallelWork* works, struct Work* w)
{
    pthread_mutex_lock(&works->mutex);
    appendQueue(&works->queue, w);
    pthread_mutex_unlock(&works->mutex);
}


static void appendListToQueue(struct LinkList* list, struct Queue* q)
{
    while (list) {
        if (list->data) {
            appendQueue(q, list->data);
        }
        list = list->next;
    }
}

struct ThreadLocal {
    pthread_t tid;
    int index;
    struct ParallelWork* work;
};

static void* threadWorker(void* arg)
{
    struct ThreadLocal* thr = (struct ThreadLocal*)(arg);
    struct ParallelWork* works = thr->work;
    struct Work* w;
    void* shared = NULL;
    if (works->tsCreate) {
        shared = works->tsCreate(thr->index);
    }

    while ((w = takeWork(works)) != NULL) {
        if (!w->thread_func(w->data, shared)) {
            w->retry += 1;
            if (w->retry == 6) {
                appendWork(works, w);
            }
        }
    }
    if (shared && works->tsFree) {
        works->tsFree(shared);
    }
    return NULL;
}

static void traverseCB(struct LinkList* node)
{
    struct Work* data = (struct Work*)node->data;
    data->retry = 0;
}

void doParallelWork(struct LinkList* work, void* (*tsCreate)(int), void (*tsFree)(void*))
{
    struct ThreadLocal* threads =
        (struct ThreadLocal*)malloc(threadCount * sizeof(struct ThreadLocal));

    struct ParallelWork workObject;
    workObject.tsCreate = tsCreate;
    workObject.tsFree = tsFree;
    initQueue(&workObject.queue);
    appendListToQueue(work, &workObject.queue);
    traverseLinkList(workObject.queue.head, traverseCB);

    pthread_mutex_init(&workObject.mutex, NULL);

    for (int i = 0; i < threadCount; i++) {
        threads[i].work = &workObject;
        threads[i].index = i;
        pthread_create(&threads[i].tid, NULL, threadWorker, threads + i);
    }

    for (int i = 0; i < threadCount; i++) {
        void* ret;
        pthread_join(threads[i].tid, &ret);
    }
    free(threads);
}

void ND_set_thread_count(int tc)
{
    threadCount = tc;
}
