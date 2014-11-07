
# lode brings shared libraries to Node.js applications
#   https://github.com/networkimprov/lode
#
# "Makefile" build shell, testglue.so, jsontest
# external dependencies: Linux, Node.js
#
# Copyright 2014 by Liam Breck


all: lode math_lode.so objects_lode.so

lode: src/shell.cc src/json.h src/atoms.h _yajl.o
	g++ -o lode -pthread -rdynamic src/shell.cc _yajl.o -ldl -Isrc -Wall

_yajl.o: $(wildcard src/yajl-2.0.4/src/*.c) $(wildcard src/yajl/*.h)
	gcc -c -o yajl_alloc.o src/yajl-2.0.4/src/yajl_alloc.c -Isrc
	gcc -c -o yajl_buf.o src/yajl-2.0.4/src/yajl_buf.c -Isrc
	gcc -c -o yajl.o src/yajl-2.0.4/src/yajl.c -Isrc
	gcc -c -o yajl_encode.o src/yajl-2.0.4/src/yajl_encode.c -Isrc
	gcc -c -o yajl_gen.o src/yajl-2.0.4/src/yajl_gen.c -Isrc
	gcc -c -o yajl_lex.o src/yajl-2.0.4/src/yajl_lex.c -Isrc
	gcc -c -o yajl_parser.o src/yajl-2.0.4/src/yajl_parser.c -Isrc
	gcc -c -o yajl_tree.o src/yajl-2.0.4/src/yajl_tree.c -Isrc
	ld -r yajl_alloc.o yajl_buf.o yajl.o yajl_encode.o yajl_gen.o yajl_lex.o yajl_parser.o yajl_tree.o -o _yajl.o

math_lode.so: demo/math_lode.cc src/lode.h src/json.h
	gcc -o math_lode.so -pthread -fpic -shared -Wl,-soname,math_lode.so demo/math_lode.cc -lm -lc -Isrc -Wall

objects_lode.so: demo/objects_lode.cc src/lode.h src/json.h
	gcc -o objects_lode.so -pthread -fpic -shared -Wl,-soname,objects_lode.so demo/objects_lode.cc -lm -lc -Isrc -Wall

testglue.so: test/testglue.cc src/lode.h src/json.h
	gcc -o testglue.so -pthread -fpic -shared -Wl,-soname,testglue.so test/testglue.cc -lm -lc -Isrc -Wall

jsontest: src/jsontest.cc src/json.h _yajl.o
	g++ -o jsontest src/jsontest.cc _yajl.o -g -Isrc -Wall
