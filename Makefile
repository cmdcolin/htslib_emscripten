ZLIB_DIR=$(PWD)/zlib
HTSLIB_DIR=$(PWD)/htslib
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    EXT=dylib
else
    EXT=so
endif
.PHONY: default
default: all;
zlib:
	cd zlib; emconfigure ./configure; emmake make; cd -

htslib: zlib
	$(MAKE) -C $(HTSLIB_DIR)

all: htslib
	emcc -pthread \
		-L $(HTSLIB_DIR) \
		-I $(ZLIB_DIR) \
		-I $(HTSLIB_DIR) \
		$(ZLIB_DIR)/libz.$(EXT) \
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
		$(HTSLIB_DIR)/hfile_net.c \
		$(HTSLIB_DIR)/faidx.c \
		$(HTSLIB_DIR)/cram/open_trace_file.c \
		htslib_mods/knetfile.c \
		htslib_mods/htsfile.c \
		-o out.html


