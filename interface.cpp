#include <emscripten.h>
#include <emscripten/fetch.h>
#include <stdlib.h>
#include <map>
#include <string>

#include "interface.h"

file_map htsFiles;
using namespace std;

extern "C" {
    // JavaScript interface
    int hts_open_js(const string& filename, int fd) {
        printf("open %s\n",filename.c_str());
        if (htsFiles.find(filename) != htsFiles.end()) return 1;

        hFILE* h_f = hopen_js(filename, fd);
        htsFile* f = hts_hopen_js(h_f, filename.c_str(), "r");
        htsFiles[filename] = f;

        return 0;
    }
    int hts_fetch_js(const string& filename) {
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");

        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_WAITABLE;
        const char * headers[]={"Range", "bytes=1-10000",0};
        attr.requestHeaders = headers; // appears to not be implemented yet
        emscripten_fetch_t *fetch = emscripten_fetch(&attr, filename.c_str());

        EMSCRIPTEN_RESULT ret = EMSCRIPTEN_RESULT_TIMED_OUT;
        while(ret == EMSCRIPTEN_RESULT_TIMED_OUT) {
            ret = emscripten_fetch_wait(fetch, 10);
        }
        if (fetch->status == 200) {
            printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
        } else {
            printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
        }
        emscripten_fetch_close(fetch);
        return -1;
    }


    // JavaScript interface
    void hts_close_js(const string& filename) {
        delete htsFiles[filename];
    }
}
