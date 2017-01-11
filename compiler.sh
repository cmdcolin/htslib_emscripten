#!/usr/bin/env bash
export ZLIB_DIR=`pwd`/zlib
mkdir dist
cd zlib
emconfigure ./configure
if [ "$(uname)" == "Darwin" ]; then
    sed -i '' "s/^AR=.*//" Makefile
    sed -i '' "s/^ARFLAGS=.*//" Makefile
fi
emmake make
cd -
cd htslib
make
if [ "$(uname)" == "Darwin" ]; then
emcc -pthread -L. -I $ZLIB_DIR -I. $ZLIB_DIR/libz.dylib libhts.dylib cram/cram_io.c hts.c hfile.c htsfile.c vcf.c sam.c bgzf.c cram/sam_header.c kstring.c md5.c cram/cram_encode.c cram/string_alloc.c cram/cram_stats.c cram/cram_codecs.c cram/thread_pool.c cram/pooled_alloc.c cram/cram_decode.c cram/rANS_static.c tbx.c cram/cram_samtools.c cram/mFILE.c cram/files.c cram/cram_index.c hfile_net.c knetfile.c faidx.c cram/open_trace_file.c -o ../dist/out.html
else
emcc -pthread -L. -I $ZLIB_DIR -I. $ZLIB_DIR/libz.so libhts.so cram/cram_io.c hts.c hfile.c htsfile.c vcf.c sam.c bgzf.c cram/sam_header.c kstring.c md5.c cram/cram_encode.c cram/string_alloc.c cram/cram_stats.c cram/cram_codecs.c cram/thread_pool.c cram/pooled_alloc.c cram/cram_decode.c cram/rANS_static.c tbx.c cram/cram_samtools.c cram/mFILE.c cram/files.c cram/cram_index.c hfile_net.c knetfile.c faidx.c cram/open_trace_file.c -o ../dist/out.html
fi
cd -
