
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "lode.js" node.js module to start lode shell and handle IPC with it
//
// Copyright 2014 by Liam Breck


var lNet = require('net');
var lChild = require('child_process');
var lFs = require('fs');


var sSocketPath = '/tmp/lode.sock';
var sShell = __dirname + '/lode';
var sSrvr = null;
var sLib = {}; // list of connected libraries
var sId = 0;   // call id counter


module.exports.load = function(iLib, iParams, iNotify) {
  if (typeof iLib !== 'string' || typeof iParams !== 'object' || typeof iNotify !== 'function')
    throw new Error('arguments are: String, Object, Function');
  if (iLib in sLib)
    throw new Error(iLib+' already loaded');

  if (!sSrvr) {
    try {
    lFs.unlinkSync(sSocketPath);
    } catch (err) {
      if (err.code !== 'ENOENT') throw err;
    }
    sSrvr = lNet.createServer(handleConnect);
    sSrvr.listen(sSocketPath, fLoad);
  } else {
    fLoad();
  }
  function fLoad() {
    sLib[iLib] = {notify:iNotify, child:null, socket:null, request:{}, quit:false};
    var aArgs = [sSocketPath, iLib];
    for (var a in iParams)
      aArgs.push(iParams[a]);
    sLib[iLib].child = lChild.spawn(sShell, aArgs, {stdio:'inherit'}); //. use iParams
    sLib[iLib].child.on('exit', function(code, signal) {
      delete sLib[iLib];
      for (var any in sLib) break;
      if (!any) {
        sSrvr.close();
        sSrvr = null;
      }
      iNotify('status', 'shell exit: '+code+' '+signal);
      iNotify('disconnect');
    });
  }
};


module.exports.call = function(iLib, iOp, iCallback) {
  if (typeof iLib !== 'string' || typeof iOp !== 'object' || typeof iCallback !== 'function')
    throw new Error('arguments are: String, Object, Function');
  if (!(iLib in sLib))
    throw new Error(iLib+' not loaded');
  if (sLib[iLib].quit)
    throw new Error(iLib+' is unloading');
  iOp._id = (++sId).toString();
  sLib[iLib].request[iOp._id] = iCallback;
  sLib[iLib].socket.write(JSON.stringify(iOp));
};


module.exports.unload = function(iLib) {
  if (typeof iLib !== 'string')
    throw new Error('arguments are: String');
  if (!(iLib in sLib))
    throw new Error(iLib+' not loaded');
  if (sLib[iLib].quit)
    throw new Error(iLib+' already unloading');
  sLib[iLib].quit = true;
  for (var any in sLib[iLib].request) break;
  if (!any)
    sLib[iLib].socket.end();
};


function handleConnect(iSoc) {
  var aTimer = setTimeout(function() {
    console.log('library connected but idle');//. disconnect
    aTimer = null;
  }, 2000);

  // feed whole json objects to JSON.parse
  // JS needs incremental json parsing!
  // if chromium would accept a patch for this, I'd fund the work.
  var aNesting=0;
  var aInStr=false, aInEsc=false;
  var aPending = [];
  var aPendingSize = 0;
  var aLib;

  iSoc.on('data', function(data) {
    for (var a1=0, a2=0; a2 < data.length; ++a2) {
      switch (data[a2]) {
      case 0x7B: // {
        if (!aInStr && ++aNesting == 1)
          a1 = a2;
        break;
      case 0x7D: // }
        if (!aInStr && --aNesting == 0) {
          var aSet;
          if (aPendingSize) {
            aPending.push(data.slice(0, a2+1));
            aSet = Buffer.concat(aPending, aPendingSize += a2+1);
            aPending = []; //. do pop(); while length; instead?
            aPendingSize = 0;
            aSet = aSet.toString('utf8');
          } else {
            aSet = data.toString('utf8', a1, a2+1);
          }
          try {
          var aParsedSet = JSON.parse(aSet);
          } catch (e) {
            e = 'unparsable msg: '+aSet+'. '+e;
            return aLib ? aLib.notify('status', e) : console.log(e);
          }
          fMsg(aParsedSet);
        }
        break;
      case 0x22: // "
        if      (!aInStr) aInStr = true;
        else if (!aInEsc) aInStr = false;
        else              aInEsc = false;
        break;
      case 0x5C: // backslash
        if (aInStr) aInEsc = !aInEsc;
        break;
      default:
        if (aInStr) aInEsc = false;
      }
    }
    if (aNesting) {
      var aAdd = a1 ? data.slice(a1) : data;
      aPendingSize += aAdd.length;
      aPending.push(aAdd);
    }
  });

  function fMsg(msg) {
    if (!aLib) {
      if (typeof msg.lib !== 'string' || !(msg.lib in sLib))
        return console.log('msg: '+data+' missing or incorrect lib member');//. ignore or disconnect
      if (aTimer)
        clearTimeout(aTimer);
      aLib = sLib[msg.lib];
      aLib.socket = iSoc;
      return aLib.notify('connect');
    } else if ('lib' in msg) {
      return aLib.notify('status', 'msg: '+data+' lib member previously given');
    } else if (typeof msg._id !== 'string' || !(msg._id in aLib.request)) {
      return aLib.notify('status', 'msg: '+data+' _id not found');
    }
    var aCall = aLib.request[msg._id];
    var aMore = '_more' in msg;
    if (aMore) {
      delete msg._more;
    } else {
      delete aLib.request[msg._id];
      if (aLib.quit) {
        for (var any in aLib.request) break;
        if (!any)
          aLib.socket.end();
      }
    }
    delete msg._id;
    aCall(null, msg, aMore);
  }

  iSoc.on('end', function() {
    if (aLib)
      aLib.notify('status', 'disconnect');
    else
      console.log('unidentified library disconnected');//. ignore
  });

}

