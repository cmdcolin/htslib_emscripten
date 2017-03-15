var ws = require("nodejs-websocket")

// Scream server example: "hi" -> "HI!!!"
var server = ws.createServer({
    selectProtocol: function(arg1, arg2) {
        console.log('a1',arg1)
        console.log('a2',arg2);
        return arg2;
    }
}, function (conn) {
    console.log("New connection")
    conn.on("text", function (str) {
        console.log("Received "+str)
        conn.sendText(str.toUpperCase()+"!!!")
    })
    conn.on("close", function (code, reason) {
        console.log("Connection closed")
    })
}).listen(8010)
