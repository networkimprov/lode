##lode
___

####*lode brings C/C++ libraries to Node.js applications*

Copyright 2014 by Liam Breck

###Rationale

Achieving the potential of a high-level language like Javascript requires making the huge population of C & C++ libraries accessible within it. Currently the common approach to this for Node.js is to create a C++ module containing "glue code" that moves data between Javascript and library calls. Such a module must use the libuv and V8 apis. 

The V8 api is arcane and poorly documented. Its use requires a lot of code. Its misuse causes subtle crash and memory-leak bugs. Implementing asynchronous calls on libuv requires before/during/after functions. Third-party glue modules may stop getting updates, or evolve in unexpected and unhelpful directions. These issues are a barrier to the possibilities of Node.js apps. 

__The lode toolkit lets C/C++ library users and developers focus on the library.__

###Mechanism

We need JS access to a C library, "libxyz"

              | app.js
    process A | libxyz.js api module: check args and form JSON messages (unique to libxyz)
              | lode.js module
              | node

        JSON messages via Domain Socket

              | lode shell
    process B | libxyz_lode.so glue code: call library, package results (unique to libxyz)
              | libxyz.so|a

The lode shell app can handle many requests concurrently, and is designed to hand-off requests to glue code without a context switch beyond any incurred by use of the socket. This mechanism should be nearly as fast as asynchronous operations within the Node thread pool.

For applications that cannot bear the modest overhead of IPC, a version of the lode shell implemented as a native Node module is envisioned. Such a scheme would also permit synchronous invocation of the library api.

###Project Status

Dependencies: Node.js v0.10.x, Linux

The lode toolkit is currently a Linux-specific prototype.

Feedback and pull requests gratefully considered!

###Getting Started

__1) Write a lode module for the library, or the parts of it which you need (demo/math_lode.cc)__

    #include <math.h>
    #include <stdio.h>
    #include <string.h>
    #include "lode.h" // the lode api; it's very simple!
    
    static unsigned char sReplyErr[] = "{'_id':'%s','error':%d}"; // response templates
    static unsigned char sReplyCos[] = "{'_id':'%s','cos':%f}";
      // requires "_id" giving the value of _id from message
      // if has "_more", lode.js will retain the callback for that _id
    
    const char* initialize(int argc, char* argv[]) {
      unsigned char* aReplies[] = { sReplyErr, sReplyCos, NULL };
      for (int i=0; aReplies[i]; ++i)
        flipQuote(aReplies[i]); // flip ' & " characters
      // initialize the library
      return NULL;
    }
    
    static const char* sQid[] = {"_id", NULL}; // JsonQuery inputs
    static const char* sQop[] = {"op", NULL};
    static const char* sQno[] = {"no", NULL};
    
    void handleMessage(JsonValue* op, ThreadQ* q) {
      JsonQuery aQ(op);
      JsonValue* aId = aQ.select(sQid).next();
      JsonValue* aOp = aQ.select(sQop).next();
      char aTmp[2048];
      int aLen=0;
      if (aOp && aOp->isString()) {
        // call library
        if (!strcmp(aOp->s.buf, "cosine")) {
          JsonValue* aNo = aQ.select(sQno).next();
          if (aNo && aNo->isDouble())
            aLen = snprintf(aTmp, sizeof(aTmp), sReplyCos, aId->s.buf, cos(aNo->d));
        }
      }
      if (aLen <= 0 || aLen > sizeof(aTmp)) {
        aLen = snprintf(aTmp, sizeof(aTmp), sReplyErr, aId->s.buf, aLen);
      }
      q->postMsg(aTmp, aLen);
      op->free();
    }

__2) Write a Node.js api for the library (demo/math.js)__

    var lode = require('../lode.js');
    var sLib = __dirname+'/../math_lode.so';
    var sEvents = {};
    
    module.exports.on = function(event, callback) {
      if (typeof event !== 'string' || typeof callback !== 'function')
        throw new Error("arguments are: String event, Function callback");
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
      lode.call(sLib, {op:'cosine',no:n}, callback);
    };

__3) Write a Node.js app using the library api (demo/mathapp.js)__

    var math = require('./math.js');
    
    math.on('connect', funtion() {
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
    
    math.init();
    
__4) Build and run__

    $ make
    $ node demo/mathapp.js

###Core Components

__lode.js__ - node.js module which spawns and talks to shell

__src/shell.cc__ - library host program

__src/yajl*__ - JSON parser, patched to allow pause/resume of parsing

__src/json.h__ - defines JSON utility api
