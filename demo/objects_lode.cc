
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "objects_lode.so" example glue library for loading by shell
//
// Copyright 2014 by Liam Breck


#include <stdio.h>
#include <string>
#include "lode.h" // the lode api; it's very simple!

class MyObject {
 public:
  MyObject(std::string data) : mData(data) { printf("creating MyObject(\"%s\")\n", mData.c_str()); }
  ~MyObject() { printf("deleting MyObject(\"%s\")\n", mData.c_str()); }
  int compare(const MyObject &other) { return mData.compare(other.mData); }
 private:
  std::string mData;
};

static char sReplyErr[] = "{'_id':'%s','error':%d}"; // response templates
static char sReplyEmpty[] = "{\"_id\":\"%s\"}";
static char sReplyMyObject[] = "{'_id':'%s','ref':%d}";
static char sReplyMyObjectCompare[] = "{'_id':'%s','compare':%d}";
  // requires "_id" giving the value of _id from message
  // if has "_more", lode.js will retain the callback for that _id

const char* initialize(int argc, char* argv[]) {
  char* aReplies[] = { sReplyErr, sReplyMyObject, sReplyMyObjectCompare, NULL };
  for (int i=0; aReplies[i]; ++i)
    flipQuote(aReplies[i]); // flip ' & " characters
  // initialize the library
  return NULL;
}

static const char* sQid[] = {"_id", NULL}; // JsonQuery inputs
static const char* sQop[] = {"op", NULL};
static const char* sQdata[] = {"data", NULL};
static const char* sQref[] = {"ref", NULL};
static const char* sQref2[] = {"ref2", NULL};
//static const char* sQno[] = {"no", NULL};

void handleMessage(JsonValue* op, ThreadQ* q) {
  JsonQuery aQ(op);
  JsonValue* aId = aQ.select(sQid).next();
  JsonValue* aOp = aQ.select(sQop).next();
  char aTmp[2048];
  int aLen=0;
  if (aOp && aOp->isString()) {
    // call library
    if (!strcmp((char*)aOp->s.buf, "MyObject")) {
      JsonValue* aData = aQ.select(sQdata).next();
      if (aData && aData->isString()) {
        MyObject* aObj = new MyObject((const char*)aData->s.buf);
        aLen = snprintf(aTmp, sizeof(aTmp), sReplyMyObject, aId->s.buf, addrToRef(aObj));
      }
    } else if (!strcmp((char*)aOp->s.buf, "MyObjectFree")) {
      JsonValue* aRefJS = aQ.select(sQref).next();
      if (aRefJS && aRefJS->isInt()) {
        MyObject* aObj = addrFromRef<MyObject>(aRefJS->i);
        delete aObj;
        aLen = snprintf(aTmp, sizeof(aTmp), sReplyEmpty, aId->s.buf);
      }
    } else if (!strcmp((char*)aOp->s.buf, "MyObjectCompare")) {
      JsonValue* aRefJS = aQ.select(sQref).next();
      JsonValue* aRefJS2 = aQ.select(sQref2).next();
      if (aRefJS && aRefJS->isInt() && aRefJS2 && aRefJS2->isInt()) {
        MyObject* aObj = addrFromRef<MyObject>(aRefJS->i);
        MyObject* aObj2 = addrFromRef<MyObject>(aRefJS2->i);
        aLen = snprintf(aTmp, sizeof(aTmp), sReplyMyObjectCompare, aId->s.buf, aObj->compare(*aObj2));
      }
    }
  }
  if (aLen <= 0 || aLen > (int)sizeof(aTmp)) {
    aLen = snprintf(aTmp, sizeof(aTmp), sReplyErr, aId->s.buf, aLen);
  }
  q->postMsg(aTmp, aLen);
}

