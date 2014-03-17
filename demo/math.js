
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "math.js" example glue library for loading by shell
//
// Copyright 2014 by Liam Breck


var lode = require('../lode.js');
var sLib = __dirname+'/../math_lode.so';
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

module.exports.cosine = function(n, callback) {
  if (typeof n !== 'number' || typeof callback !== 'function')
    throw new Error('arguments are: Number n, Function callback');
  lode.call(sLib, {op:'cosine',no:n}, callback);
};

