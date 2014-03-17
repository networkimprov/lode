
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "atoms.h" posix-portable atomic types based on sem_t
// todo: replace semaphore with atomic functions as in http://golubenco.org/2007/06/14/atomic-operations/
//
// Copyright 2014 by Liam Breck


#include <semaphore.h>


class AtomicCounter {
public:
  AtomicCounter(unsigned int i=0) { sem_init(&m, 0, i); }
  ~AtomicCounter() { sem_destroy(&m); }

  operator unsigned int() { int a; sem_getvalue(&m, &a); return a; }
  AtomicCounter& operator++() { sem_post(&m); return *this; }
  AtomicCounter& operator--() { sem_trywait(&m); return *this; }

private:
  AtomicCounter(const AtomicCounter&);
  AtomicCounter& operator=(const AtomicCounter&);
  sem_t m;
};


class AtomicBool {
public:
  AtomicBool(bool i=false) { sem_init(&m, 0, i); }
  ~AtomicBool() { sem_destroy(&m); }

  operator bool() { int a; sem_getvalue(&m, &a); return a; }
  AtomicBool& operator=(bool i) { sem_trywait(&m); if (i) sem_post(&m); return *this; }

private:
  AtomicBool(const AtomicBool&);
  AtomicBool& operator=(const AtomicBool&);
  sem_t m;
};

