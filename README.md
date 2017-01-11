# htslib_emscripten

Making htslib compile to javascript

## Prerequisites

- Emscripten, with emcc, emmake, emconfigure in path
- LLVM and clang compiler for emscripten
- Optional, java, for google closure compiler

The htslib and zlib dependencies are available in this repo as git subtrees

## Build

    ./compiler.sh

Will output dist/out.html and dist/out.js which can be opened in a webbrowser

## Notes

This can compile htsfile.c accurately, but it is not usable due to blocking sockets not being supported by emscripten (AFAIK)

Compilation tested on Ubuntu and OSX
