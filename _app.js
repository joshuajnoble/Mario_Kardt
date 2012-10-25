
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

var wiflyCarts = [];
var controllers = [];
var controllerCartJoint = [];
var lastIndex = 0;
var currentGameState = {}; // what modifier it is, and who did it

app.get('/', routes.index);
app.get('/play', function(req, res) {

   //var url_parts = url.parse(req.url, true);
   var query = req.query;

// we have a color!
   /*if(query['color']) {
      currentGameState['event'] = query.color;
      currentGameState['source'] = query.id;

      setTimeout( function() {
          currentGameState = null;
      }, 2000); // 2 seconds of madness?

   } 
   else if(currentGameState) 
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
   } else {
*/
    var currentController = null;
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
    
    if(currentController.left < 10) {
      left = "00" + currentController.left.toString();
    } else if(currentController.left < 100) {
      left = "0" + currentController.left.toString();
    } else {
      left = currentController.left.toString();
    }

    if(currentController.right < 10) {
      right = "00" + currentController.right.toString();
    } else if(currentController.right < 100) {
      right = "0" + currentController.right.toString();
    } else {
      right = currentController.right.toString();
    }
    

    res.write([left, right].join(":"));
  } else {
    res.write("122:122");
  }
   res.end();

});

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

      /*var k = 0;
      var foundKey;
      for(var key in controllers) {
        if(k == lastIndex) {
          console.log(" found index " + k.toString() + " " + key);
          foundKey = key;
          break;
        }
        k++;
      }
      */
      var newCart = {id:lastIndex};
      wiflyCarts.push(newCart);
      //console.log(lastIndex + " " + wiflyCarts[lastIndex]);

   }
   res.setHeader("Content-Type", "text/html");
   res.write(lastIndex.toString());
   res.end();

   lastIndex++;

});

function pairControllerToCart () {
  if(controllerCartJoint.length < controllers.length && controllerCartJoint.length < wiflyCarts.length) {
    controllerCartJoint.push( {contoller:controllers[controllers.length-1], cart:wiflyCarts[wiflyCarts.length-1]} );
  }
};

// create server
var server = http.createServer(app).listen(app.get('port'), function(){
  console.log("Express server listening on port " + app.get('port'));
});

// SOCKET CODE

var io = sio.listen(server);

io.sockets.on('connection', function (socket) {
  console.log(socket.id);

  var controller = {id:socket.id, left: 255, right: 255 };
  controllers.push(controller);

  pairControllerToCart();

  socket.on('left', function (data) {

    controller.left = data;
    console.log(controllers[socket.id]);
  });

  socket.on('right', function (data) {
    controller.right = data;
    console.log(controllers[socket.id]);
  });

  socket.on('disconnect', function () {
    console.log('disconnect = ' + socket.id);
    var controllerId = controllers.indexOf(controller);
    delete controllers[controllerId];

  });

});
