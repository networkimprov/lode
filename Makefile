
# lode brings shared libraries to Node.js applications
#   https://github.com/networkimprov/lode
#
# "Makefile" build shell, testglue.so, jsontest
# external dependencies: Linux, Node.js
#
# Copyright 2014 by Liam Breck


all: lode math_lode.so

lode: src/shell.cc src/json.h src/atoms.h yajl.o
	g++ -o lode -pthread -rdynamic src/shell.cc yajl.o -ldl -Isrc -Wall

yajl.o: $(wildcard src/yajl-2.0.4/src/*.c) $(wildcard src/yajl/*.h)
	gcc -o yajl.o -combine -c $(wildcard src/yajl-2.0.4/src/*.c) -Isrc

math_lode.so: demo/math_lode.cc src/lode.h src/json.h
	gcc -o math_lode.so -pthread -fpic -shared -Wl,-soname,math_lode.so demo/math_lode.cc -lm -lc -Isrc -Wall

testglue.so: test/testglue.cc src/lode.h src/json.h
	gcc -o testglue.so -pthread -fpic -shared -Wl,-soname,testglue.so test/testglue.cc -lm -lc -Isrc -Wall

jsontest: src/jsontest.cc src/json.h yajl.o
	g++ -o jsontest src/jsontest.cc yajl.o -g -Isrc -Wall

