#include "nd.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <errno.h>

#include <spdlog/spdlog.h>
#include "utils.h"

#define HTML "/tmp/jjwxc.html"


void* loadHTML(size_t& size)
{
    struct stat st;
    int s = stat(HTML, &st);
    int fd = open(HTML, O_RDONLY);
    if (fd < 0 || s == -1) {
        std::cerr << "Open " HTML " failed." << strerror(errno) << std::endl;
        exit(1);
    }
    auto ret = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ret == nullptr) {
        std::cerr << "mmap failed. " << strerror(errno) << std::endl;
        exit(2);
    }
    close(fd);
    size = st.st_size;
    return ret;
}

void unloadHTML(void* data, size_t size)
{
    munmap(data, size);
}
int main()
{
    size_t s;
    auto data = loadHTML(s);

    spdlog::set_level(spdlog::level::trace);
    ND_set_log_function(logger_func);

    struct JJwxc jj;
    ND_init();
    ND_jjwxc_doit_buffer(data, s, &jj);

    unloadHTML(data, s);
    saveNovel(&jj.n);

    ND_jjwxc_free(&jj);
    ND_shutdown();
}