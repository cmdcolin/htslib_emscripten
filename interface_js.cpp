#include <emscripten.h>
#include <string>

#include "interface.h"
#include "pileup.h" 

#include "hts_js.h"

using namespace std;

void callback_pileup(const char *chrom, int pos, char ref_base, int depth) {
    EM_ASM_ARGS({
        postMessage([2, Pointer_stringify($0), $1, String.fromCharCode($2), $3]); // The first argument is for message classification, 0 and 1 are already reserved internally
    }, chrom, pos, ref_base, depth);
}

extern "C" {
    int run_pileup(string fd_bam, string fd_bai, string reg) {
        hts_idx_t *bai;

        bai = hts_idx_load_js(htsFiles[fd_bai]);
        printf("run_pileup cpp\n");

        pileup(htsFiles[fd_bam], bai, reg.c_str(), callback_pileup);

        if (bai) hts_idx_destroy_js(bai);
        return 0;
    }
}
