
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "objects.js" example glue library for loading by shell
//
// Copyright 2014 by Liam Breck


var lode = require('../lode.js');
var lLR = require('../build/Release/node_loderef.node');
var sLib = __dirname+'/../objects_lode.so';
var sEvents = {};

module.exports.on = function(event, callback) {
  if (typeof event !== 'string' || typeof callback !== 'function')
    throw new Error('arguments are: String event, Function callback');
  sEvents[event] = callback;
};

module.exports.init = function(params) {
  lode.load(sLib, params, function(op, data) {
    if (sEvents[op])
      return sEvents[op](data);
    if (op === 'error')
      throw data;
  });
};

module.exports.quit = function() {
  lode.unload(sLib);
}

module.exports.MyObject = function(s, callback) {
  if (typeof s !== 'string' || typeof callback !== 'function')
    throw new Error('arguments are: String s, Function callback');
  lode.call(sLib, {op:'MyObject',data:s}, function(err, data) {
    if (err) { callback(err); return; }
    callback(null, new MyObjectClass(data.ref));
  });
};

function MyObjectClass(ref) {
  this.type = 'MyObject';
  this.ref = ref;
  this.lodeRef = lLR.LodeRef(function(){
    lode.call(sLib, {op:'MyObjectFree', ref:ref}, function(){});
  });
}

MyObjectClass.prototype.compare = function(o, callback) {
  if (o.type !== 'MyObject')
    throw new Error('arguments are: MyObject o, Function callback');
  lode.call(sLib, {op:'MyObjectCompare', ref: this.ref, ref2: o.ref}, function(err, data) {
    if (err) { callback(err); return; };
    callback(err, data.compare);
  });
}
