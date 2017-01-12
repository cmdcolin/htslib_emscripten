# htslib_emscripten

Making htslib compile to javascript

## Prerequisites

- Emscripten, with emcc, emmake, emconfigure in path
- LLVM and clang compiler for emscripten
- Optional, java, for google closure compiler

The htslib and zlib dependencies are available in this repo as git subtrees

## Build

    ./compiler.sh

Will output js/Store/SeqFeature/CRAM/Htslib.js which can be used in javascript as in js/Store/SeqFeature/CRAM/File.js 

## Notes

Compilation tested on Ubuntu and OSX

This does not produce anything functional at the moment but demonstrates calling htslib functions from javascript
