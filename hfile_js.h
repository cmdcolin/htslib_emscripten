#include "htslib/hfile.h"
#include "hfile_internal.h"

typedef struct {
    hFILE base;
    char * filename;
} hFILE_js;

hFILE *hopen_js(int);
