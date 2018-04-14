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
	void downloadSucceeded(emscripten_fetch_t *fetch) {
	  printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
	  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
	  emscripten_fetch_close(fetch); // Free data associated with the fetch.
	}

	void downloadFailed(emscripten_fetch_t *fetch) {
	  printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
	  emscripten_fetch_close(fetch); // Also free data on failure.
	}
    int hts_fetch_js(const string& filename) {
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);

		strcpy(attr.requestMethod, "GET");

		const char * headers[]={"Range", "bytes=1-1000",0};
        attr.requestHeaders = headers; // appears to not be implemented yet

		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
        attr.onsuccess = downloadSucceeded;
        attr.onerror = downloadFailed;
		emscripten_fetch_t *fetch = emscripten_fetch(&attr, "volvox-sorted.bam"); // Blocks here until the operation is complete.
        return -1;
    }


    // JavaScript interface
    void hts_close_js(const string& filename) {
        delete htsFiles[filename];
    }
}
