#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "cram/cram.h"
#include "htslib/sam.h"
#include "htslib/hfile.h"

int main(int argc, char *argv[])
{
    samFile *in = 0;
    bam_hdr_t *h;
    bam1_t *b;
    htsFile *out;
    int r = 0, exit_code = 0;
    hts_opt *in_opts = NULL, *out_opts = NULL;
    int nreads = 0;
    
    hts_idx_t *idx;
    char *filename = "http://localhost/jbrowse/plugins/CramReader/test/data/volvox.cram";
    hFILE *hfp = hopen(filename, "r");
    printf("%p", hfp);
    in = hts_hopen(hfp, filename, "r");
    idx = sam_index_load(in, "test.cram");
    printf("%p", idx);
    if (in == NULL) {
        fprintf(stderr, "Error opening \"%s\"\n", "test.cram");
        return EXIT_FAILURE;
    }
    h = sam_hdr_read(in);
    if (h == NULL) {
        fprintf(stderr, "Couldn't read header for \"%s\"\n", "test.cram");
        return EXIT_FAILURE;
    }
    h->ignore_sam_err = 0;

    b = bam_init1();

    out = hts_open("-", "w");


    // Process any options; currently cram only.
    if (hts_opt_apply(in, in_opts))
        return EXIT_FAILURE;
    hts_opt_free(in_opts);

    if (hts_opt_apply(out, out_opts))
        return EXIT_FAILURE;
    hts_opt_free(out_opts);

    if (sam_hdr_write(out, h) < 0) {
        fprintf(stderr, "Error writing output header.\n");
        exit_code = 1;
    }
    if (optind + 1 < argc) { // BAM input and has a region
        int i;
        hts_idx_t *idx;
        if ((idx = sam_index_load(in, "test.cram")) == 0) {
            fprintf(stderr, "[E::%s] fail to load the BAM index\n", __func__);
            return 1;
        }
        for (i = optind + 1; i < argc; ++i) {
            hts_itr_t *iter;
            if ((iter = sam_itr_querys(idx, h, argv[i])) == 0) {
                fprintf(stderr, "[E::%s] fail to parse region '%s'\n", __func__, argv[i]);
                continue;
            }
            while ((r = sam_itr_next(in, iter, b)) >= 0) {
                if (sam_write1(out, h, b) < 0) {
                    fprintf(stderr, "Error writing output.\n");
                    exit_code = 1;
                    break;
                }
                if (nreads && --nreads == 0)
                    break;
            }
            hts_itr_destroy(iter);
        }
        hts_idx_destroy(idx);
    } else while ((r = sam_read1(in, h, b)) >= 0) {
        if (sam_write1(out, h, b) < 0) {
            fprintf(stderr, "Error writing output.\n");
            exit_code = 1;
            break;
        }
        if (nreads && --nreads == 0)
            break;
    }

    if (r < -1) {
        fprintf(stderr, "Error parsing input.\n");
        exit_code = 1;
    }

    r = sam_close(out);
    if (r < 0) {
        fprintf(stderr, "Error closing output.\n");
        exit_code = 1;
    }

    bam_destroy1(b);
    bam_hdr_destroy(h);

    r = sam_close(in);
    if (r < 0) {
        fprintf(stderr, "Error closing input.\n");
        exit_code = 1;
    }

    return exit_code;
}
