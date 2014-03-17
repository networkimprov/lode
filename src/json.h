
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "json.h" yajl callbacks to construct json tree, and class to query a tree
// dependencies: yajl patched to allow complete_parse after client_cancel
//
// Copyright 2014 by Liam Breck


#include <string.h>
#include <stdlib.h>

#include "yajl/yajl_parse.h"

#ifndef JSON_NEST_MAX
#define JSON_NEST_MAX 128
#endif

char* flipQuote(char* ioStr) {
  for (int a=0; ioStr[a]; ++a)
    switch (ioStr[a]) {
      case '\'': ioStr[a] = '"';  break;
      case '"' : ioStr[a] = '\''; break;
    }
  return ioStr;
}

struct JsonValue {
  inline bool isNull()   { return type == eNull; }
  inline bool isInt()    { return type == eInt;  }
  inline bool isDouble() { return type == eDbl;  }
  inline bool isBool()   { return type == eBool; }
  inline bool isString() { return type == eStr;  }
  inline bool isArray()  { return type == eList && e.type == 1; }
  inline bool isObject() { return type == eList && e.type == 0; }

  void free(int* k=0, int* v=0);

  struct Ent {
    Ent* next;
    JsonValue* val;
    char type; // 1=idx, 0=key
    union {
      unsigned int idx;
      unsigned char key[1]; // variable length
    };
  };

  enum Type { eNull, eInt, eDbl, eBool, eStr, eList } type;
  union {
    Ent e;
    long long i;
    double d;
    bool b;
    struct {
      size_t len;
      unsigned char buf[1]; // variable length
    } s;
  };

private:
  JsonValue();
  JsonValue(const JsonValue&);
};

void JsonValue::free(int* oKeyN, int* oValN) {
  if (type == eList) {
    if (e.val)
      e.val->free(oKeyN, oValN);
    while (e.next) {
      Ent* a = e.next;
      e.next = a->next;
      a->val->free(oKeyN, oValN);
      ::free(a);
      if (oKeyN) *oKeyN -= 1;
    }
  }
  ::free(this);
  if (oValN) *oValN -= 1;
}

static const char* sWildcard = "*";

class JsonQuery {
public:
  JsonQuery(JsonValue* iVal) {
    mVal = iVal;
  }

  JsonQuery& select(const char* iQuery[]) {
    mTailStack[mTailN = 0] = mVal->type == JsonValue::eList ? & mVal->e : NULL;
    for (int a=0; iQuery[a]; mQuery[++a] = NULL) {
      if (a == JSON_NEST_MAX)
        throw 2;
      mQuery[a] = !strcmp(iQuery[a], sWildcard) ? sWildcard : iQuery[a];
      if (mQuery[a][0] == sWildcard[0] && !strcmp(mQuery[a]+1, sWildcard))
        ++mQuery[a];
    }
    return *this;
  }

  JsonValue* next() {
    while (mTailStack[mTailN]) {
      JsonValue::Ent* aE = mTailStack[mTailN];
      bool aMatch = aE->val && ( mQuery[mTailN] == sWildcard ||
          (aE->type == 1 ?
            strtoul(mQuery[mTailN], NULL, 10) == aE->idx :
            strcmp(mQuery[mTailN], (char*)aE->key) == 0) );
      if (aMatch && aE->val->type == JsonValue::eList && mQuery[mTailN+1]) {
        mTailStack[++mTailN] = & aE->val->e;
        continue;
      }
      aMatch = aMatch && mQuery[mTailN+1] == NULL;
      while (mTailN && !mTailStack[mTailN]->next)
        --mTailN;
      mTailStack[mTailN] = mTailStack[mTailN]->next;
      if (aMatch)
        return aE->val;
    }
    return NULL;
  }

private:
  JsonQuery();
  JsonQuery(const JsonQuery&);

  JsonValue* mVal;
  const char* mQuery[JSON_NEST_MAX+1];
  JsonValue::Ent* mTailStack[JSON_NEST_MAX];
  int mTailN;
};

static JsonValue::Ent sNewlist;

class JsonTree {
public:
  JsonTree(int (*iCb)(JsonValue*)) {
    mCallback = iCb;
    mRoot.val = NULL;
    mTailStack[mTailN = 0] = &mRoot;
    mKeyN = mValN = 0;
  }

  ~JsonTree() {
    if (mRoot.val)
      mRoot.val->free(&mKeyN, &mValN);
  }

  static const yajl_callbacks sCallbacks;
  int mKeyN, mValN;

private:
  int (*mCallback)(JsonValue*);
  JsonValue::Ent mRoot;
  JsonValue::Ent* mTailStack[JSON_NEST_MAX];
  int mTailN;

  void newKey(const unsigned char* iBuf = NULL, size_t iLen = 0) {
    JsonValue::Ent* aE;
    if (mTailStack[mTailN]->next == &sNewlist) {
      mTailStack[mTailN]->next = NULL;
      aE = & newValue(JsonValue::eList, iLen) -> e;
    } else {
      ++mKeyN;
      aE = (JsonValue::Ent*) malloc(sizeof(JsonValue::Ent) + iLen);
      aE->next = NULL;
      aE->val = NULL;
    }
    aE->type = iLen == 0;
    if (aE->type) {
      aE->idx = mTailStack[mTailN]->idx + 1;
    } else {
      memcpy(aE->key, iBuf, iLen);
      aE->key[iLen] = '\0';
    }
    if (mTailStack[mTailN] != aE)
      mTailStack[mTailN] = mTailStack[mTailN]->next = aE;
  }

  JsonValue* newValue(JsonValue::Type iType, size_t iStrLen = 0) {
    ++mValN;
    JsonValue* aV = (JsonValue*) malloc(sizeof(JsonValue) + iStrLen);
    aV->type = iType;
    if (mTailStack[mTailN]->val) // append to array
      newKey();
    mTailStack[mTailN]->val = aV;
    if (iType == JsonValue::eList) {
      aV->e.val = NULL;
      aV->e.next = NULL;
      if (++mTailN == JSON_NEST_MAX)
        throw 1;
      mTailStack[mTailN] = & aV->e;
    }
    return aV;
  }

  int valueResult() {
    if (mTailN == 0) {
      JsonValue* aV = mRoot.val;
      mRoot.val = NULL;
      return mCallback(aV);
    }
    return 1;
  }

  static int onObject(void* ioJt) {
    JsonTree* that = (JsonTree*) ioJt;
    that->mTailStack[that->mTailN]->next = &sNewlist;
    return 1;
  }

  static int onArray(void* ioJt) {
    JsonValue* aJv = ((JsonTree*) ioJt)->newValue(JsonValue::eList);
    aJv->e.type = 1;
    aJv->e.idx = 0;
    return 1;
  }

  static int onEndList(void* ioJt) {
    JsonTree* that = (JsonTree*) ioJt;
    if (that->mTailStack[that->mTailN]->next == &sNewlist) {
      that->mTailStack[that->mTailN]->next = NULL;
      JsonValue* aV = that->newValue(JsonValue::eList);
      aV->e.type = 0;
      aV->e.key[0] = '\0';
    }
    -- that->mTailN;
    return that->valueResult();
  }

  static int onKey(void* ioJt, const unsigned char* iBuf, size_t iLen) {
    ((JsonTree*) ioJt)->newKey(iBuf, iLen);
    return 1;
  }

  static int onNull(void* ioJt) {
    ((JsonTree*) ioJt)->newValue(JsonValue::eNull);
    return ((JsonTree*) ioJt)->valueResult();
  }

  static int onBool(void* ioJt, int iVal) {
    JsonValue* aV = ((JsonTree*) ioJt)->newValue(JsonValue::eBool);
    aV->b = iVal;
    return ((JsonTree*) ioJt)->valueResult();
  }

  static int onInt(void* ioJt, long long iVal) {
    JsonValue* aV = ((JsonTree*) ioJt)->newValue(JsonValue::eInt);
    aV->i = iVal;
    return ((JsonTree*) ioJt)->valueResult();
  }

  static int onDbl(void* ioJt, double iVal) {
    JsonValue* aV = ((JsonTree*) ioJt)->newValue(JsonValue::eDbl);
    aV->d = iVal;
    return ((JsonTree*) ioJt)->valueResult();
  }

  static int onStr(void* ioJt, const unsigned char* iBuf, size_t iLen) {
    JsonValue* aV = ((JsonTree*) ioJt)->newValue(JsonValue::eStr, iLen);
    memcpy(aV->s.buf, iBuf, aV->s.len = iLen);
    aV->s.buf[aV->s.len] = '\0';
    return ((JsonTree*) ioJt)->valueResult();
  }
};

const yajl_callbacks JsonTree::sCallbacks = {
  onNull, onBool,
  onInt, onDbl, NULL,
  onStr,
  onObject, onKey, onEndList,
  onArray, onEndList
};


