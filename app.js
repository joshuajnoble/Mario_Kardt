
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

app.get('/', routes.index);

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