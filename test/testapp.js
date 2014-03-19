
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

  var aCount=0;
  tg.getJoy('mental', cb);
  function cb(err, joy, more) {
    if (err) throw err;
    console.log('got joy: '+joy.data);
    if (more)
      return;
    if (++aCount === 4)
      tg.quit();
    else
      setTimeout(tg.getJoy, 2000, 'sensual', cb);
  }
});

tg.on('status', function(data) {
  console.log(data);
});

tg.on('disconnect', function() {
  console.log("library disconnected");
});

tg.init([101,280]);

