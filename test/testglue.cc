
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "testglue.so" example glue library for loading by shell
//
// Copyright 2014 by Liam Breck


#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "lode.h"


const char* initialize(int argc, char* argv[]) {
  int* aI = new int;
  *aI = 101010101;
  int aRef = addrToRef(aI);
  aI = addrFromRef<int>(aRef);
  printf("ref %d, value %d\n", aRef, *aI);
  printf("glue lib init'd\n");
  return 0;
}

static const char* sQid[] = {"_id", NULL};
static const char* sQfn[] = {"fn", NULL};

void handleMessage(JsonValue* iV, ThreadQ* oQ) {
  double aD = cos(1.0); // verify dynamic lib loaded

  JsonQuery aQ(iV);
  JsonValue* aId = aQ.select(sQid).next();
  JsonValue* aFn = aQ.select(sQfn).next();
  fprintf(stdout, "handleMessage fn %s, id %s\n", aFn ? (char*)aFn->s.buf : "no fn", aId ? (char*)aId->s.buf : "no id");

  char aM1[] = "{'_id':'%s','data':'{msg rec\\'d\\\\', '_more':1}";
  char aM2[] = "{'_id':'%s','data':'";
  char aM3[] = "msg rec\\'d}\\\\', '_more':1}{'_id':'%s','data':'";
  char aM4[] = "msg rec\\'d}\\\\', '_more':";
  char aM5[] = "1}{'_id':'%s','data':'{msg rec\\'d\\\\', '_more':1}{'_id':'%s',";
  char aM6[] = "'data':'{msg rec\\'d}\\\\'}";
  char* aList[] = {aM1, aM2, aM3, aM4, aM5, aM6};
  char aResp[256];
  for (int a=0; a < 6; ++a) {
    oQ->postMsg(aResp, snprintf(aResp, sizeof(aResp), flipQuote(aList[a]), aId->s.buf, aId->s.buf));
    usleep(10000);
  }
  iV->free();
}

