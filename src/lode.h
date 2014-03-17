
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "lode.h" api for library modules
//
// Copyright 2014 by Liam Breck


#include "json.h"


class ThreadQ {
public:
  void postMsg(const char*, size_t);
};

extern "C"
void handleMessage(JsonValue* op, ThreadQ* q); // must call q->postMsg() and op->free()

extern "C"
const char* initialize(int argc, char* argv[]); // optional

