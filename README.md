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

## websock_server

The application relies on communicating CRAM results over websockets, so a lightweight websocket server is provided

### Usage

    yarn
    node websock_server

You may want to use `pm2` or `forever` to run the package and/or reverse proxy over nginx

## Notes

Compilation tested on Ubuntu and OSX


