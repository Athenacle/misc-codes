#include "core.h"

#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>

static size_t writeCB(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;

    struct Buffer *buf = (struct Buffer *)(userp);
    appendBuffer(buf, data, realsize);

    return realsize;
}

void client_init(struct HttpClient *hc)
{
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCB);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, ND_random_ua());
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    hc->curl = curl;
}

void client_fetch(URL url, struct HttpClient *hc, struct CurlResponse *resp)
{
    CURL *curl = hc->curl;
    CURLcode res;
    struct Buffer buf;
    initBuffer(&buf);

    size_t size = strlen(url) + 128;
    char *msg = (char *)malloc(size);

    memset(resp, 0, sizeof(*resp));

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, url);

    resp->htmlLength = resp->status = 0;
    resp->html = resp->responseHeader = NULL;

    res = curl_easy_perform(curl);
    res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        resp->html = collectBuffer(&buf, &resp->htmlLength);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status);
        snprintf(msg,
                 size,
                 "Get URL %s successfully Status Code %d, ContentLength: %ld",
                 url,
                 resp->status,
                 totalSize(&buf));
        DEBUG(msg);
    } else {
        snprintf(msg, size, "Get URL %s failed, error %s.", url, curl_easy_strerror(res));
        ERROR(msg);
    }

    clearBuffer(&buf);
    free(msg);
}

void client_free(struct HttpClient *hc)
{
    CURL *curl = (CURL *)(hc->curl);
    curl_easy_cleanup(curl);
}

void fetch(URL url, struct CurlResponse *resp)
{
    struct Buffer buf;

    CURL *curl = curl_easy_init();
    size_t size = strlen(url) + 128;
    char *msg = (char *)malloc(size);

    initBuffer(&buf);
    memset(resp, 0, sizeof(*resp));

    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCB);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, ND_random_ua());

    res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        resp->html = collectBuffer(&buf, &resp->htmlLength);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp->status);
        snprintf(msg,
                 size,
                 "Get URL %s successfully Status Code %d, ContentLength: %ld",
                 url,
                 resp->status,
                 totalSize(&buf));
        DEBUG(msg);
    } else {
        snprintf(msg, size, "Get URL %s failed, error %s.", url, curl_easy_strerror(res));
        ERROR(msg);
    }
    curl_easy_cleanup(curl);
    clearBuffer(&buf);
    free(msg);
}