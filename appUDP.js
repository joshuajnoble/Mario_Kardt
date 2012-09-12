
/**
 * Module dependencies.
 */

var express = require('express')
  , routes = require('./routes')
  , http = require('http')
  , sio = require('socket.io')
  , dgram = require('dgram');

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

  // just make sure that we're not updating too fast
  setInterval(function() {
    updateGame(); // try to only update the game 10xSec
    if(Math.abs(time - Date.now()) > 100) {
      time = Date.now();
    }
  }, 100);

});

app.configure('development', function(){
  app.use(express.errorHandler());
});

var wiflyCarts = [];
var controllers = [];
var controllerCartJoint = [];
var lastIndex = 0;
var currentGameState = {}; // what modifier it is, and who did it
var time = Date.now();

// udp socket
var udpSocket = dgram.createSocket('udp4');

udpSocket.on("listening", function () {
  var address = udpSocket.address();
  console.log("server listening " + address.address + ":" + address.port);
});

udpSocket.on("message", function(msg, rinfo) {
  var messageStr = msg.toString();
  var idInd = messageStr.indexOf("id=");
  var colorInd = messageStr.indexOf("color=");
  var id, color;
  if(idInd != -1 && colorInd != -1) {
    id = messageStr.slice(idInd, idInd+1);
    color = messageStr.slice(colorInd, idInd+1);
  }
  currentGameState['event'] = color;
  currentGameState['source'] = id;

  setTimeout( function() {
      currentGameState = null;
  }, 2000); // 2 seconds of madness?

});

udpSocket.bind(8001);

//app.get('/', routes.index);
function updateGame() {

   if(currentGameState) 
   {
      switch(currentGameState['event']) {
      	case 0:
      // nada, normal game play
      	break;
      	case 1:
      // if we're not the one who sent it, then it affects us
      	if(currentGameState['source'] != query.id) {

      	    carts[wiflyCarts[query.id]].left = 10; // slow down, for example
      	    carts[wiflyCarts[query.id]].right = 10; // slow down, for example
      	}
      	break;

      	case 2:
        if(currentGameState['source'] == query.id) {

            // how to do faster
        }
      	break;
      	case 3:
        // make everybody else do a loop
        if(currentGameState['source'] != query.id) {

            carts[wiflyCarts[query.id]].left = 255; // slow down, for example
            carts[wiflyCarts[query.id]].right = -255; // slow down, for example
        }
      	break;
      	case 4:
        // make everybody else skew left
        if(currentGameState['source'] != query.id) {
          //
        }
      	break;
      	case 5:
        // make everybody else skew right?
        if(currentGameState['source'] != query.id) {
          //
        }
      	break;
      	case 6:
        // one more thing?
        if(currentGameState['source'] != query.id) {
          //
        }
      	break;
        default:
        break;
      }
   }

    /*var currentController = null;
    var left;
    var right;
    controllerCartJoint.forEach( function( obj, index, arr) {
      if(obj.cart.id == query.id) {
        currentController = obj.controller; 
      }
    });
   //}
   res.setHeader("Content-Type", "text/html");
   if(currentController) {
  
    res.write([left, right].join(":"));
  } else {
    res.write("122:122");
  }
   res.end();*/

}

function stringify( l, r ){
  var left = "", right = "";
  if(l < 10) {
      left = "00" + currentController.left.toString();
    } else if(currentController.left < 100) {
      left = "0" + currentController.left.toString();
    } else {
      left = currentController.left.toString();
    }

    if(r < 10) {
      right = "00" + currentController.right.toString();
    } else if(currentController.right < 100) {
      right = "0" + currentController.right.toString();
    } else {
      right = currentController.right.toString();
    }
    return left + ":" + right;
}

function pairControllerToCart () {
  if(controllerCartJoint.length < controllers.length && controllerCartJoint.length < wiflyCarts.length) {
    controllerCartJoint.push( {contoller:controllers[controllers.length-1], cart:wiflyCarts[wiflyCarts.length-1]} );
  }
}

app.get('/id', function(req,res) {

   if(!wiflyCarts[lastIndex]) {

      // strictly for testing
      controllers["xyxyxyz"] = {
        left: 255,
        right: 255
      };

      controllers["xyxyxya"] = {
        left: 122,
        right: 122
      };

      var newCart = {id:lastIndex};
      wiflyCarts.push(newCart);
      //console.log(lastIndex + " " + wiflyCarts[lastIndex]);

   }
   res.setHeader("Content-Type", "text/html");
   res.write(lastIndex.toString());
   res.end();

   lastIndex++;

});

// create server
var server = http.createServer(app).listen(app.get('port'), function(){
  console.log("Express server listening on port " + app.get('port'));
});

// SOCKET CODE
var io = sio.listen(server);

// now we send messages to everyone
io.sockets.on('connection', function (socket) {
  console.log(socket.id);

  var controller = {id:socket.id, left: 255, right: 255 };
  controllers.push(controller);

  pairControllerToCart();

  socket.on('left', function (data) {

    controller.left = data;
    console.log(controllers[socket.id]);

    var msgStr = "";
    controllerCartJoint.forEach( function( obj, index, arr) {
      msgStr += obj.cart.id + "=" + stringify(obj.controller.left, obj.controller.right) + ";";
    });

    var buf = Buffer(msgStr.split(""));
    udpSocket.send(buf, 0, buf.length, 3001, "0.0.0.0");
    time = Date.now();

  });

  // now we send messages to everyone
  socket.on('right', function (data) {
    controller.right = data;
    console.log(controllers[socket.id]);

    var msgStr = "";
    controllerCartJoint.forEach( function( obj, index, arr) {
      msgStr += obj.cart.id + "=" + stringify(obj.controller.left, obj.controller.right) + ";";
    });

    var buf = Buffer(msgStr.split(""));
    udpSocket.send(buf, 0, buf.length, 3001, "0.0.0.0");
    time = Date.now();
  });

  socket.on('disconnect', function () {
    console.log('disconnect = ' + socket.id);
    var controllerId = controllers.indexOf(controller);
    delete controllers[controllerId];

  });

});
