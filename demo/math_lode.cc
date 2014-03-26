
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "math_lode.so" example glue library for loading by shell
//
// Copyright 2014 by Liam Breck


#include <math.h>
#include <stdio.h>
#include <string.h>
#include "lode.h" // the lode api; it's very simple!

static char sReplyErr[] = "{'_id':'%s','error':%d}"; // response templates
static char sReplyCos[] = "{'_id':'%s','cos':%f}";
  // requires "_id" giving the value of _id from message
  // if has "_more", lode.js will retain the callback for that _id

const char* initialize(int argc, char* argv[]) {
  char* aReplies[] = { sReplyErr, sReplyCos, NULL };
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
    if (!strcmp((char*)aOp->s.buf, "cosine")) {
      JsonValue* aNo = aQ.select(sQno).next();
      if (aNo && (aNo->isDouble() || aNo->isInt()))
        aLen = snprintf(aTmp, sizeof(aTmp), sReplyCos, aId->s.buf, cos(aNo->isInt() ? aNo->i * 1.0 : aNo->d));
    }
  }
  if (aLen <= 0 || aLen > (int)sizeof(aTmp)) {
    aLen = snprintf(aTmp, sizeof(aTmp), sReplyErr, aId->s.buf, aLen);
  }
  q->postMsg(aTmp, aLen);
}

