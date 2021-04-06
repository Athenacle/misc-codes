#include "websites.h"

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define WEBSITE_REGEX "^https?://www\\.52shuku\\.vip/"

static int check(URL url)
{
    return regex_match(url, WEBSITE_REGEX);
}

/*
static void sk52v_detail(struct CurlResponse* resp, struct Novel* n) {}
*/

static void doit(URL url, struct CurlResponse* resp, struct Novel* n)
{
    /*
    if (regex_match(url, WEBSITE_REGEX)) {
        INFO("shuku52vip novel detail.");
        sk52v_detail(resp, n);
    }*/
}


struct WebsiteHandler shuku52vip = {.check = check, .doIt = doit, .name = "shuku52vip"};
