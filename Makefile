HTSLIB_DIR=$(PWD)/htslib
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    EXT=dylib
else
    EXT=so
endif

default: all



all:
	cd $(HTSLIB_DIR); autoheader; autoconf; ./configure; $(MAKE); cd -
	emcc -pthread \
		-L $(HTSLIB_DIR) \
		-I $(HTSLIB_DIR) \
		-s USE_ZLIB=1 \
		$(HTSLIB_DIR)/libhts.$(EXT) \
		$(HTSLIB_DIR)/cram/cram_io.c \
		$(HTSLIB_DIR)/hts.c \
		$(HTSLIB_DIR)/hfile.c \
		$(HTSLIB_DIR)/vcf.c \
		$(HTSLIB_DIR)/sam.c \
		$(HTSLIB_DIR)/bgzf.c \
		$(HTSLIB_DIR)/cram/sam_header.c \
		$(HTSLIB_DIR)/kstring.c \
		$(HTSLIB_DIR)/md5.c \
		$(HTSLIB_DIR)/cram/cram_encode.c \
		$(HTSLIB_DIR)/cram/string_alloc.c \
		$(HTSLIB_DIR)/cram/cram_stats.c \
		$(HTSLIB_DIR)/cram/cram_codecs.c \
		$(HTSLIB_DIR)/cram/thread_pool.c \
		$(HTSLIB_DIR)/cram/pooled_alloc.c \
		$(HTSLIB_DIR)/cram/cram_decode.c \
		$(HTSLIB_DIR)/cram/rANS_static.c \
		$(HTSLIB_DIR)/tbx.c \
		$(HTSLIB_DIR)/cram/cram_samtools.c \
		$(HTSLIB_DIR)/cram/mFILE.c \
		$(HTSLIB_DIR)/cram/files.c \
		$(HTSLIB_DIR)/cram/cram_index.c \
		$(HTSLIB_DIR)/hfile.c \
		$(HTSLIB_DIR)/hfile_net.c \
		$(HTSLIB_DIR)/knetfile.c \
		$(HTSLIB_DIR)/faidx.c \
		$(HTSLIB_DIR)/cram/open_trace_file.c \
        pileup.cpp \
        interface_js.cpp \
		interface.cpp \
		--post-js postfix.js \
		-s NO_EXIT_RUNTIME=1 \
		-s FORCE_FILESYSTEM=1 \
        -s USE_PTHREADS=1 \
        -s EXPORT_ALL=1 \
        -s FETCH=1 \
        -o pileup_bam.js


