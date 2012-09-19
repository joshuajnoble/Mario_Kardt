/*var WebSocketServer = require('ws').Server
  , wss = new WebSocketServer({port: 3000});
wss.on('connection', function(ws) 
{

	ws.on('message', function(message) {
		console.log('received: %s', message);
		ws.send('something');
	});

	ws.on('close', function() {
		console.log('stopping client interval');
		clearInterval(id);
	});

});*/

var WebSocketServer = require('ws').Server
  , http = require('http')
  , express = require('express')
  , app = express.createServer();

app = express();
app.listen(3000);

var wss = new WebSocketServer({server: app});
wss.on('connection', function(ws) {
  var id = setInterval(function() {
    ws.send(JSON.stringify(process.memoryUsage()), function() {  });
  }, 100);
  console.log('started client interval');
  ws.on('close', function() {
    console.log('stopping client interval');
    clearInterval(id);
  })
});