#include "nd.h"
#include "utils.h"

#include <spdlog/spdlog.h>

#include <iostream>

#include <argparse/argparse.hpp>
#include <curl/curl.h>

using namespace spdlog::level;

namespace
{
    inline level_enum dispatchLevel(int lv)
    {
        level_enum lvs[] = {off, err, warn, info, debug, trace};
        if (lv >= 0 && lv <= 5) {
            return lvs[lv];
        } else {
            return spdlog::level::off;
        }
    }
}  // namespace

namespace novel
{
    void set_log_level(int lv)
    {
        spdlog::set_level(dispatchLevel(lv));
    }


    void logger_func(int level, const char* msg)
    {
        auto l = dispatchLevel(level);
        spdlog::log(l, msg);
    }

    void saveNovel(struct Novel* n)
    {
        if (n->title) {
            std::string title(n->title);
            title.append(".txt");

            FILE* fp = fopen(title.c_str(), "w");
            if (fp == nullptr) {
                std::cerr << "Open file " << title << " failed: " << strerror(errno) << std::endl;
            } else {
                char* nc = ND_collect_novel(n);
                if (nc) {
                    fprintf(fp, "%s", nc);
                    ND_free_collected_buffer(nc);
                }
                fclose(fp);
            }
        }
    }

    void uploadNovel(struct Novel* n, const novel::Flags& flag)
    {
        chdir("/tmp");
        saveNovel(n);
        std::string f = fmt::format("{}.txt", n->title);

        auto c = curl_easy_init();
        curl_mime* form = curl_mime_init(c);


        {
            auto fi = curl_mime_addpart(form);
            curl_mime_name(fi, "path");
            curl_mime_data(fi, "/", 1);
        }
        {
            auto field = curl_mime_addpart(form);
            curl_mime_name(field, "newfile");
            curl_mime_type(field, "text/plain");
            curl_mime_filename(field, f.c_str());
            curl_mime_filedata(field, f.c_str());
        }

        curl_easy_setopt(c, CURLOPT_URL, fmt::format("{}/upload", flag.upload).c_str());
        curl_easy_setopt(c, CURLOPT_POST, 1);
        curl_easy_setopt(c, CURLOPT_MIMEPOST, form);

        auto r = curl_easy_perform(c);
        if (r == CURLE_OK) {
            spdlog::info("Upload to {} success", flag.upload);
        } else {
            spdlog::error("Upload to {} failed: {}", flag.upload, curl_easy_strerror(r));
        }

        unlink(f.c_str());
        curl_easy_cleanup(c);
    }


    bool parseArgument(Flags& f, int argc, const char** argv)
    {
#if defined VERSION  && defined GIT_HASH
        argparse::ArgumentParser prog(argv[0], VERSION "-" GIT_HASH);
#else
        argparse::ArgumentParser prog(argv[0]);
#endif

        prog.add_argument("-p", "--proxy")
            .help("use proxy in curl format.")
            .default_value(
                std::string(
#ifdef DEFAULT_PROXY
            DEFAULT_PROXY 
#else
                ""
#endif
                ));

        prog.add_argument("-u", "--upload").help("upload").default_value(std::string(
#ifdef DEFAULT_UPLOAD
            DEFAULT_UPLOAD
#else
            ""
#endif        
            ));

        prog.add_argument("url").help("URL").required();

        try {
            prog.parse_args(argc, argv);
        } catch (const std::exception& err) {
            std::cerr << err.what() << std::endl;
            std::cerr << prog;
            std::exit(1);
        }

        f.proxy = prog.get<std::string>("--proxy");
        f.upload = prog.get<std::string>("--upload");
        f.url = prog.get<std::string>("url");
        return true;
    }
}  // namespace novel
