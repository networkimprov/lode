#ifndef _NODE_LODEREF_
#define _NODE_LODEREF_

#include <v8.h>
#include <node.h>

using namespace v8;
using namespace node;

class LodeRef : public node::ObjectWrap {
public:
  static void Init(v8::Handle<v8::Object> exports);
private:
  LodeRef() {};
  ~LodeRef();
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Persistent<v8::Function> constructor;
  Persistent<Function> callback;
};

#endif //_NODE_LODEREF_
