# htslib_emscripten

Making htslib compile to javascript

## Prerequisites

- Emscripten, with emcc, emmake, emconfigure in path
- LLVM and clang compiler for emscripten
- Optional, java, for google closure compiler
- Node.js for websock_server

The htslib and zlib dependencies are available in this repo as git subtrees

## Build

    make
## Notes

Compilation tested on Ubuntu and OSX

**This project is currently totally non-functional**
