var ws = require("nodejs-websocket")
var http = require('http');

// Scream server example: "hi" -> "HI!!!"
var server = ws.createServer({
    selectProtocol: function(arg1, arg2) {
        return 'binary';//arg2;
    }
}, function (conn) {
    console.log("New connection")
    var out = conn.beginBinary()
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
            console.log(data.toString())
            //console.log(data.toString());
            var options = {
                hostname: 'www.google.com',
                port: 80,
                method: 'GET'
            };
            console.log("!!!!!!!!!!!!!!!!!");
            var proxy = http.request(options, function (res) {
                //console.log(res)
                console.log(`STATUS: ${res.statusCode}`);
                console.log(`HEADERS: ${JSON.stringify(res.headers)}`);
                res.setEncoding('utf8');
                res.on('data', (chunk) => {
                    console.log(chunk)
                  out.write(chunk)
                  console.log(`BODY: ${chunk}`);
                });
                res.on('end', () => {
                  console.log('No more data in response.');
                });
            });
            proxy.end();
        })
    })
    conn.on("close", function (code, reason) {
        console.log("Connection closed")
    })
}).listen(8010)
