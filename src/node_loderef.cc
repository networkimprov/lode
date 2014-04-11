#include "node_loderef.h"

void InitAll(Handle<Object> exports) {
  LodeRef::Init(exports);
}
NODE_MODULE(node_loderef, InitAll)

Persistent<Function> LodeRef::constructor;

void LodeRef::Init(Handle<Object> exports) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("LodeRef"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  constructor = Persistent<Function>::New(tpl->GetFunction());
  exports->Set(String::NewSymbol("LodeRef"), constructor);
}

Handle<Value> LodeRef::New(const Arguments& args) {
  HandleScope scope;
  if (args.IsConstructCall()) {
    // Invoked as constructor: `new MyObject(...)`
    if (args.Length() != 1 || !args[0]->IsFunction())
      return ThrowException(v8::Exception::TypeError(String::New("Arguments are (Function)")));
    LodeRef* obj = new LodeRef();
    obj->callback = Persistent<Function>::New(Handle<Function>::Cast(args[0]));
    obj->Wrap(args.This());
    return args.This();
  } else {
    // Invoked as plain function `LodeRef(...)`, turn into construct call.
    if (args.Length() != 1)
      return ThrowException(v8::Exception::TypeError(String::New("Arguments are (Function)")));
    const int argc = 1;
    Local<Value> argv[argc] = { args[0] };
    return scope.Close(constructor->NewInstance(argc, argv));
  }
}

LodeRef::~LodeRef() {
  callback->Call(handle_, 0, NULL);
  callback.Dispose();
}
