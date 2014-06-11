var lr = require('../build/Release/node_loderef.node');

var aCbCalled = false
function Func() {
  new lr.LodeRef(function(){ aCbCalled = true; });
}

Func();
for (var i = 0; i < 200; i++)
  new Buffer(1000000);

if (aCbCalled)
  console.log('OK - Callback on destructor called');
else
  console.log('NOK - Callback on destructor not called');

