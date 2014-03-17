
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "mathapp.js" example glue library for loading by shell
//
// Copyright 2014 by Liam Breck


var math = require('./math.js');

math.on('connect', function() {
  console.log('math library ready');
  
  var aN = 42;
  math.cosine(aN, function cb(err, data, more) {
    if (err) throw err;
    console.log('cosine of '+aN+' is '+data.cos);
    if (!more)
      setTimeout(math.cosine, 2000, ++aN, cb);
  });
});

math.on('disconnect', function() {
  console.log('math library quit');
});

math.init(null);

