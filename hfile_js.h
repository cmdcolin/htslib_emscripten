#include "htslib/hfile.h"
#include "hfile_internal.h"
#include <string>

typedef struct {
    hFILE base;
    std::string filename;
} hFILE_js;

hFILE *hopen_js(const std::string&);
