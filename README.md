# htslib_emscripten

Making htslib compile to javascript

## Prerequisites

- Emscripten, with emcc, emmake, emconfigure in path
- LLVM and clang compiler for emscripten
- Optional, java, for google closure compiler

The htslib code is available in this repo as a git subtree

## Build

    make

## Notes

This repo has incorporated code from https://github.com/eilslab/htslib-js and shares the LGPL license

Compilation tested on Ubuntu and OSX

**This project is currently totally non-functional**


