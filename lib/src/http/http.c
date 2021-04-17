#include "core.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <curl/curl.h>

#define HPARSER_OPTION (HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR | HTML_PARSE_RECOVER)

static void initCurlResponseData(struct CurlResponse *resp)
{
    if (resp->type != TEXT_HTML) {
        initBuffer(&resp->data.buf);
    }
}

int inputHttpParser(struct HtmlParser *parser, const void *data, int size)
{
    if (parser->ctx == NULL) {
        parser->ctx = htmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL, XML_CHAR_ENCODING_NONE);
        htmlCtxtUseOptions(parser->ctx, HPARSER_OPTION);
    }
    if (size == 0) {
        int ret = htmlParseChunk(parser->ctx, data, size, 1);
        parser->doc = parser->ctx->myDoc;
        return ret;
    } else if (size > 0) {
        return htmlParseChunk(parser->ctx, data, size, 0);
    } else {
        int ret = htmlParseChunk(parser->ctx, data, -1 * size, 1);
        parser->doc = parser->ctx->myDoc;
        return ret;
    }
}

static size_t curlWriteDataCB(void *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;

    struct CurlResponse *resp = (struct CurlResponse *)(userp);

    resp->contentLength += realsize;

    if (resp->type == TEXT_HTML) {
        inputHttpParser(&resp->data.parser, data, realsize);
    } else {
        appendBuffer(&resp->data.buf, data, realsize);
    }

    return realsize;
}


#define CONTENT_TYPE "content-type:"

#define CT_TEXT_HTML "text/html"
#define CT_IMAGE_JPEG "image/jpeg"
#define CT_APP_JSON "application/json"
#define CT_TEXT_PLAIN "text/plain"


static size_t headerStrlen(char *str)
{
    char c;
    size_t ret = 0;
    while ((c = *str)) {
        if (c == '\r') {
            return ret;
        } else {
            str++;
        }
    }
    return 0;
}
#define MIN(a, b) (((((hs = ((a)))) <= ((b))) ? ((hs)) : ((b))))  // check function curlHeaderCB
#define MATCH(in, text) (strncasecmp(in, text, MIN(headerStrlen(in), sizeof(text) - 1)) == 0)

static size_t curlHeaderCB(char *b, size_t size, size_t nitems, void *userdata)
{
    size_t numbytes = size * nitems;
    size_t hs = 0;

    struct CurlResponse *resp = (struct CurlResponse *)userdata;

    if (MATCH(b, CONTENT_TYPE)) {
        char *hv = b + sizeof(CONTENT_TYPE);
        char c;
        while ((c = *hv) != '\r') {
            if (unlikely(isblank(c) || c == ':')) {
                hv++;
            } else {
                break;
            }
        }

        if (likely(MATCH(hv, CT_TEXT_HTML))) {
            resp->type = TEXT_HTML;
        } else if (likely(MATCH(hv, CT_IMAGE_JPEG))) {
            resp->type = IMAGE_JPEG;
        } else if (MATCH(hv, CT_APP_JSON)) {
            resp->type = APP_JSON;
        } else if (MATCH(hv, CT_TEXT_PLAIN)) {
            resp->type = TEXT_PLAIN;
        } else {
            char *lb = getCoreTempBuffer();
            for (int i = 0; i < CORE_BUFFER_SIZE; i++) {
                c = *hv;
                if (c == 0xd && hv[1] == 0xa) {
                    lb[i] = 0;
                } else {
                    lb[i] = c;
                }
            }
            resp->type = CT_NONE;
            resp->contentType = strdup(lb);
            freeCoreTempBuffer(lb);
        }
        initCurlResponseData(resp);
    }

    return numbytes;
}
#undef MATCH
#undef MIN

static void initCurl(CURL *curl)
{
    TRACE_EXPR(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteDataCB), CURLE_OK);
    TRACE_EXPR(curl_easy_setopt(curl, CURLOPT_USERAGENT, ND_random_ua()), CURLE_OK);
    TRACE_EXPR(curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlHeaderCB), CURLE_OK);
    TRACE_EXPR(curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L), CURLE_OK);
    TRACE_EXPR(curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""), CURLE_OK);
}

void initHttpClient(struct HttpClient *hc)
{
    CURL *curl = curl_easy_init();
    TRACE_EXPR(curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L), CURLE_OK);
    initCurl(curl);

    hc->curl = curl;
}

#define SUCCESS_FMT "Get URL %s successfully Status Code %d, ContentLength: %ld"
#define FAILED_FMT "Get URL %s failed, error %s."


void fetchClient(URL url, struct HttpClient *hc, struct CurlResponse *resp)
{
    CURL *curl = hc->curl;
    CURLcode res;
    char *msg;

    SET_ZERO(resp);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)resp);

    res = curl_easy_perform(curl);
    if (resp->type == TEXT_HTML) {
        inputHttpParser(&(resp->data.parser), NULL, 0);
    }

    msg = getCoreTempBuffer();

    if (res == CURLE_OK) {
        int status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

        resp->status = status;
        snprintf(msg, CORE_BUFFER_SIZE, SUCCESS_FMT, url, status, resp->contentLength);
        DEBUG(msg);
    } else {
        snprintf(msg, CORE_BUFFER_SIZE, FAILED_FMT, url, curl_easy_strerror(res));
        ERROR(msg);
    }
    freeCoreTempBuffer(msg);
}

void freeClient(struct HttpClient *hc)
{
    CURL *curl = (CURL *)(hc->curl);
    curl_easy_cleanup(curl);
}

void fetch(URL url, struct CurlResponse *resp)
{
    struct HttpClient hc;
    initHttpClient(&hc);
    fetchClient(url, &hc, resp);
    freeClient(&hc);
}
