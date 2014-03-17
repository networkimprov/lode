
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "testapp.js" example node.js app calling example glue module
// usage: $ node testapp.js
//
// Copyright 2014 by Liam Breck


var tg = require('./testglue.js');

tg.on('connect', function() {
  console.log("library ready");

  tg.getJoy('mental', cb);
  function cb(err, joy, more) {
    if (err) throw err;
    console.log('got joy: '+joy.data);
    if (!more)
      setTimeout(tg.getJoy, 2000, 'sensual', cb);
  }
});

tg.on('disconnect', function() {
  console.log("library disconnected");
});

tg.init([101,280]);

