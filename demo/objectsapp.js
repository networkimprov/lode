
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "objectsapp.js" example glue library for loading by shell
//
// Copyright 2014 by Liam Breck

var objects = require('./objects.js');

objects.on('connect', function() {
  console.log('objects library ready');
  objects.MyObject('obj1', function(err, obj1) {
    if (err) throw err;
    objects.MyObject('obj2', function(err, obj2) {
      if (err) throw err;
      obj1.compare(obj2, function (err, result){
        if (err) throw err;
        console.log('obj1.compare(obj2) = ' + result);
      });
      obj1.compare(obj1, function (err, result){
        if (err) throw err;
        console.log('obj1.compare(obj1) = ' + result);
      });
    });
  });  
});

objects.on('disconnect', function() {
  console.log('objects library quit');
});

setTimeout(function() {
  for (var i = 0; i < 200; i++)
    new Buffer(1000000);
  objects.quit();
}, 1000);

objects.init(null);

