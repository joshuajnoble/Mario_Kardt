#!/usr/bin/env node
var app = require('express')()
  , server = require('http').createServer(app);

server.listen(3000);

app.get('/', function (req, res) {
  res.sendfile(__dirname + '/index.html');
});
