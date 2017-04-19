# Notes

## websock_server

The application relies on communicating CRAM results over websockets, so a lightweight websocket server is provided

### Usage

    yarn
    node websock_server

You may want to use `pm2` or `forever` to run the package and/or reverse proxy over nginx

