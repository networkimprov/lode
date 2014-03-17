
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "testglue.js" example library API node.js module calling lode.js
//
// Copyright 2014 by Liam Breck


var lode = require('../lode.js');

var sLib = __dirname+'/../testglue.so';
var sEvents = {};

module.exports.on = function(event, callback) {
  if (typeof event !== 'string' || typeof callback !== 'function')
    throw new Error('arguments are: String, Function');
  sEvents[event] = callback;
};

module.exports.init = function(parameters) {
  lode.load(sLib, parameters, function(op, data) {
    if (sEvents[op])
      return sEvents[op](data);
    if (op === 'error')
      throw data;
  });
};

module.exports.getJoy = function(type, callback) {
  lode.call(sLib, {fn:'getJoy', arg:type}, callback);
};

