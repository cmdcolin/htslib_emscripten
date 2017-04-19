# Notes


Approaches to using HTSLIB on client side

1) You can try to use the code to get htsfile but the way that emscripten works is that socket code in C is converted into websocket requests, so even though htslib can make "http requests" these will be converted into websocket requests, so you need a websocket server to process them. That is the purpose of the node.js websock_server code in this repository.

2) You can try to not use any of the htslib socket code, and instead try and call htslib functions that process data directly, and then you could potentially make plain AJAX requests for the data in javascript and pass them to htslib functions. Unfortunately, this approach may be very challenging also because htslib code requires a lot of special structures to be made, and it is difficult to make this work.

Of the two approaches, I think approach #1 would be easiest but it requires at a minimum an extra dependency to handle websocket requests on the server side




## websock_server

The application relies on communicating CRAM results over websockets, so a lightweight websocket server is provided

### Usage

    yarn
    node websock_server

You may want to use `pm2` or `forever` to run the package and/or reverse proxy over nginx

