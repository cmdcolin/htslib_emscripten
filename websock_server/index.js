var ws = require("nodejs-websocket")

// Scream server example: "hi" -> "HI!!!"
var server = ws.createServer({
    selectProtocol: function(arg1, arg2) {
        console.log('a2',arg2);
        console.log('a1',arg1);
        console.log(arg1.buffer.toString());
        return 'binary';//arg2;
    }
}, function (conn) {
    console.log("New connection")
    conn.on("binary", function (inStream) {
        // Empty buffer for collecting binary data 
        var data = new Buffer(0)
        // Read chunks of binary data and add to the buffer 
        inStream.on("readable", function () {
            var newData = inStream.read()
            if (newData)
                data = Buffer.concat([data, newData], data.length+newData.length)
        })
        inStream.on("end", function () {
            console.log("Received " + data.length + " bytes of binary data")
            console.log(data.toString());
            var options = {
                hostname: 'www.google.com',
                port: 80,
                method: 'GET'
            };

            var proxy = http.request(options, function (res) {
                res.pipe(conn, {
                    end: true
                });
            });

        })
    })
    conn.on("close", function (code, reason) {
        console.log("Connection closed")
    })
}).listen(8010)
