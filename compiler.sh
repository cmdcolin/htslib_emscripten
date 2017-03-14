#!/usr/bin/env bash
export ZLIB_DIR=`pwd`/zlib
export TEST_FILE=htsfile.c
if [ "$(uname)" == "Darwin" ]; then
EXT=dylib
else
EXT=so
fi

mkdir -p dist

# configure zlib
#cd zlib
#emconfigure ./configure
#emmake make
#cd -

# confgure htslib
cd htslib
make
emcc -pthread \
    -L. \
    -I $ZLIB_DIR \
    -I. \
    $ZLIB_DIR/libz.$EXT \
    libhts.$EXT cram/cram_io.c \
    hts.c \
    hfile.c \
    $TEST_FILE \
    vcf.c \
    sam.c \
    bgzf.c \
    cram/sam_header.c \
    kstring.c \
    md5.c \
    cram/cram_encode.c \
    cram/string_alloc.c \
    cram/cram_stats.c \
    cram/cram_codecs.c \
    cram/thread_pool.c \
    cram/pooled_alloc.c \
    cram/cram_decode.c \
    cram/rANS_static.c \
    tbx.c \
    cram/cram_samtools.c \
    cram/mFILE.c \
    cram/files.c \
    cram/cram_index.c \
    hfile_net.c \
    knetfile.c \
    faidx.c \
    cram/open_trace_file.c \
    -o ../dist/out.html
    #--embed-file vcf.c \
    #--pre-js ../prefix.js \
    #--post-js ../postfix.js \
    #-s NO_EXIT_RUNTIME=1 \
    #-s FORCE_FILESYSTEM=1 \
    #-s EXPORTED_FUNCTIONS="['_hts_open','_sam_hdr_read','_bam_init1','_sam_read1','_bam_aux_get','_bam_aux2A','_sam_index_load']"
cd -
cp dist/out.js js/Store/SeqFeature/CRAM/Htslib.js
