
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "jsontest" exercise json.h
//
// Copyright 2014 by Liam Breck


#include <string.h>
#include <stdio.h>

#include <json.h>

static char sIn[] = " \
  [ {'a':1, 'b':null, 'c':1.2, 'd':'str'}, [], {}, {'a':'a\"\\'b'} ] \
  { 'a':[], 'b':{'1':1, '2':[]}, 'c':'str', 'a':2 } \
";

static int sQueryN = -1;
static const char* sQuery[] = {
  "*", "a", NULL,
  "a", NULL
};

static int on_msg(JsonValue*);
static JsonTree sTree(on_msg);

static int on_msg(JsonValue* iVal) {
  printf("count %d,%d ", sTree.mKeyN, sTree.mValN);
  JsonQuery aQ(iVal);
  aQ.select(sQuery + ++sQueryN);
  printf("query");
  while (sQuery[sQueryN])
    printf(" %s", sQuery[sQueryN++]);
  printf(" result: ");
  JsonValue* aV;
  while (aV = aQ.next()) {
    switch (aV->type) {
    case JsonValue::eInt:  printf("%lld ", aV->i); break;
    case JsonValue::eDbl:  printf("%f ", aV->d); break;
    case JsonValue::eStr:  printf("%s ", aV->s.buf); break;
    case JsonValue::eNull: printf("null ");      break;
    case JsonValue::eBool: printf("%d ", aV->b); break;
    }
  }
  iVal->free(&sTree.mKeyN, &sTree.mValN);
  return 0;
}

int main(int argc, char* argv[]) {
  yajl_handle aH = yajl_alloc(&JsonTree::sCallbacks, NULL, &sTree);
  yajl_config(aH, yajl_allow_multiple_values, 1);

  yajl_status aS = yajl_parse(aH, (unsigned char*)flipQuote(sIn), sizeof(sIn)-1);
  while (aS == yajl_status_client_canceled) {
    printf("s%d \n", aS);
    aS = yajl_complete_parse(aH);
  }
  if (aS != yajl_status_ok)
    return printf("yajl error %s\n", yajl_get_error(aH, 1, NULL, 0));

  printf("final count: %d,%d\n", sTree.mKeyN, sTree.mValN);

  return 0;
}

