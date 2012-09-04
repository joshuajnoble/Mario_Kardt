
/**
 * Module dependencies.
 */

var express = require('express')
  , routes = require('./routes')
  , http = require('http')
  , sio = require('socket.io');

var app = express();

app.configure(function(){
  app.set('port', process.env.PORT || 3000);
  app.set('views', __dirname + '/views');
  app.set('view engine', 'ejs');
  app.use(express.favicon());
  app.use(express.logger('dev'));
  app.use(express.bodyParser());
  app.use(express.methodOverride());
  app.use(express.cookieParser('your secret here'));
  app.use(express.session());
  app.use(app.router);
  app.use(require('less-middleware')({ src: __dirname + '/public' }));
  app.use(express.static(__dirname + '/public'));
});

app.configure('development', function(){
  app.use(express.errorHandler());
});

var cartClients = {};
var lastIndex = 0;
var currentGameModifier = {}; // what modifier it is, and who did it.

//app.get('/', routes.index);
app.get('/play', function(req, res) {

   var url_parts = url.parse(request.url, true);
   var query = url_parts.query;

// we have a color!
   if(query['color']) {
      currentGameModifier['event'] = query.color;
      currentGameModifier['source'] = query.id;
   } else {
      switch(currentGameModifier['event']) {
	case 0:
// nada, normal game play
	break;
	case 1:
// if we're not the one who sent it, then it affects us
	if(currentGameModifier != query.id) {

	    carts[cartClients[query.id]].left = 10; // slow down, for example
	    carts[cartClients[query.id]].right = 10; // slow down, for example
	}
	break;
	case 2:
	break;
	case 3:
	break;
	case 4
	break;
	case 5:
	break;
	case 6:
	break;
      }
   }
   res.setHeader("Content-Type", "text/html");
   res.write(carts[cartClients[query.id]]);
   res.end();

});

app.get('/id', function(req,res) {

   if(!cartClient[lastIndex]) {
       cartClient[lastIndex] = carts[lastIndex]; 
   }
   res.setHeader("Content-Type", "text/html");
   res.write(lastIndex);
   res.end();
   lastIndex++;
});

// create server
var server = http.createServer(app).listen(app.get('port'), function(){
  console.log("Express server listening on port " + app.get('port'));
});

// SOCKET CODE

var io = sio.listen(server),
    carts = {};

io.sockets.on('connection', function (socket) {
  console.log(socket.id);

  carts[socket.id] = {
    left: 255,
    right: 255
  };

  socket.on('left', function (data) {
    carts[socket.id].left = data;
    console.log(carts[socket.id]);
  });

  socket.on('right', function (data) {
    carts[socket.id].right = data;
    console.log(carts[socket.id]);
  });

  socket.on('disconnect', function () {
    console.log('disconnect = ' + socket.id);
    delete carts[socket.id];
  });

});
