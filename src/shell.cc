
// lode brings C/C++ libraries to Node.js applications
//   https://github.com/networkimprov/lode
//
// "shell" executable to host shared library and glue code and handle IPC with lode.js
// todo: connect to multiple node.js processes, quit threads after inactivity
//
// Copyright 2014 by Liam Breck


#include <dlfcn.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "atoms.h"
#include "json.h"


static int sFd; // socket to parent process
static int sPollFd; // socket to epoll
static const int kTimeout = 60*1000; // interval between checks for idle threads

static yajl_handle sParser; // instance of json parser
static JsonValue* sJsonMsg = NULL; // msg storage until passed to handleMessage
static int onJsonMsg(JsonValue* i) { sJsonMsg = i; return 0; }
static JsonTree sTree(onJsonMsg);

static unsigned int kThreadMax=16; // max number of started threads
static AtomicCounter sThreadCount(1); // number of started threads
static class ThreadQ* sThreads; // list of threads

static void* threadLoop(void*); // thread function

typedef void (*HandleMessagePtr)(JsonValue*, ThreadQ*);
static HandleMessagePtr handleMessage; // library-specific message processor, defined in shared libraries
typedef const char* (*InitializePtr)(int, char**);
static InitializePtr initialize; // optional library init function

char gEquator; // used by addrToRef & addrFromRef //. use int if all memory refs aligned that way

static int errno_exit(const char* s) { perror(s); exit(1); return 0; }


class ThreadQ {
public:
  ThreadQ() {}
  ~ThreadQ();

  bool start(bool iNew = true) {
    mHead = mTail = NULL;
    errno = pthread_mutex_init(&mQchg, NULL);
    errno && errno_exit("ThreadQ::start mutex_init");
    if (!iNew)
      return !threadLoop(this);
    errno = pthread_create(&mT, NULL, threadLoop, this);
    if (errno)
      perror("ThreadQ::start pthread_create");
    return !errno;
  }

  void postMsg(const char* iB, size_t iL);

  static void drainQueues(bool iAddQ = false);

private:
  ThreadQ(const ThreadQ&);

  pthread_t mT;
  pthread_mutex_t mQchg;

  struct Qent {
    Qent* next;
    size_t len;
    char buf[sizeof(int)]; // attempt to use padding
  } *mHead, *mTail;
};

void ThreadQ::postMsg(const char* iB, size_t iL) {
  Qent* aI = (Qent*) malloc(sizeof(Qent)-sizeof(int)+iL);
  memcpy(aI->buf, iB, aI->len = iL);
  aI->next = NULL;
  errno = pthread_mutex_lock(&mQchg);
  errno && errno_exit("ThreadQ::queueMsg mutex_lock");
  bool aAddQ = !mHead;
  if (!mHead)
    mHead = mTail = aI;
  else
    mTail = mTail->next = aI;
  pthread_mutex_unlock(&mQchg);
  drainQueues(aAddQ);
}


int main(int argc, char* argv[]) {

  if (argc < 3) {
    fprintf(stderr, "usage: %s socketpath library.so\n", argv[0]);
    return 1;
  }

  void* aLib = dlopen(argv[2], RTLD_NOW);
  if (aLib) {
    initialize = (InitializePtr) dlsym(aLib, "initialize");
    dlerror(); // clear preceding error
    handleMessage = (HandleMessagePtr) dlsym(aLib, "handleMessage");
  }
  const char* aErr = dlerror();
  if (aErr) {
    fprintf(stderr, "dl error %s\n", aErr);
    return 1;
  }
  if (initialize) {
    aErr = initialize(argc-3, argv+3);
    if (aErr) {
      fprintf(stderr, "%s initialize error %s\n", argv[2], aErr);
      return 1;
    }
  }

  sFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
  sFd < 0 && errno_exit("main socket");

  sPollFd = epoll_create(1);
  sPollFd < 0 && errno_exit("main epoll_create");

  epoll_event aEv = { EPOLLIN };
  epoll_ctl(sPollFd, EPOLL_CTL_ADD, sFd, &aEv)
    && errno_exit("main epoll_ctl");

  sockaddr_un aAddr = { AF_UNIX, "" };
  strncpy(aAddr.sun_path, argv[1], sizeof(aAddr.sun_path));
  connect(sFd, (sockaddr*) &aAddr, sizeof(sockaddr_un))
    && errno_exit("main connect");

  char aMsg[64];
  write(sFd, aMsg, snprintf(aMsg, sizeof(aMsg), "{\"lib\":\"%s\"}", argv[2])) < 0
    && errno_exit("main write");

  sParser = yajl_alloc(&JsonTree::sCallbacks, NULL, &sTree);
  yajl_config(sParser, yajl_allow_multiple_values, 1);

  sThreads = new ThreadQ[kThreadMax];
  sThreads[0].start(false);

  return 0;
}


static void* threadLoop(void* oT) {
  static pthread_mutex_t sBaton = PTHREAD_MUTEX_INITIALIZER;
  static AtomicCounter sBusyCount(0); // occupied threads
  static unsigned char sMsgBuf[8*1024];

  while (true) {
    errno = pthread_mutex_lock(&sBaton); // most threads wait here
    errno && errno_exit("threadLoop mutex_lock");
    ++sBusyCount;

    int aReady, aSelN=0;
    do {
      if (sJsonMsg) {
        sJsonMsg = NULL;
        if (yajl_complete_parse(sParser) == yajl_status_error) {
          fprintf(stderr, "parser error %s\n", yajl_get_error(sParser, 1, NULL, 0));
          exit(1);
        }
        if (sJsonMsg)
          break;
      }
      epoll_event aEv;
      aReady = epoll_wait(sPollFd, &aEv, 1, kTimeout); // one thread waits here
      aReady < 0 && errno_exit("threadLoop epoll_wait");
      if (aReady == 0) {
        //. terminate excess threads
      } else if (aEv.events & EPOLLOUT) {fprintf(stderr, "pollout\n");
        aEv.events = EPOLLIN;
        epoll_ctl(sPollFd, EPOLL_CTL_MOD, sFd, &aEv)
          && errno_exit("threadLoop epoll_ctl");
        ThreadQ::drainQueues();
        aReady = 0;
      } else if (aEv.events & EPOLLIN) {fprintf(stderr, "pollin\n");
        int aSize;
        do {
          aSize = read(sFd, sMsgBuf, sizeof(sMsgBuf));
          if (aSize < 0) {
            errno != EWOULDBLOCK && errno_exit("threadLoop read");
          } else if (aSize == 0) {
            exit(0);
          } else if (yajl_parse(sParser, sMsgBuf, aSize) == yajl_status_error) {
            fprintf(stderr, "parser error %s\n", yajl_get_error(sParser, 1, NULL, 0));
            exit(1);
          }
        } while (!sJsonMsg && !(aSize < 0 && errno == EWOULDBLOCK));
        aReady = sJsonMsg ? 1 : 0;
      } else if (aEv.events & EPOLLHUP) {
        fprintf(stderr, "epoll_wait got hangup\n");
        exit(0); 
      } else {
        fprintf(stderr, "epoll_wait event not handled %d\n", aEv.events);
        aReady = 0;
      }
      ++aSelN; /// track wakes
    } while (aReady < 1);
    if (aSelN > 1) fprintf(stdout, "select count %d\n", aSelN);///

    JsonValue* aMsg = sJsonMsg;
    unsigned int aTot = sThreadCount;
    if (aTot < kThreadMax && sBusyCount == aTot && sThreads[aTot].start())
      ++sThreadCount;
    pthread_mutex_unlock(&sBaton);
    handleMessage(aMsg, (ThreadQ*)oT);
    --sBusyCount;
  }
  return NULL;
}


void ThreadQ::drainQueues(bool iAddQ) {
  static pthread_mutex_t sM = PTHREAD_MUTEX_INITIALIZER;
  static AtomicCounter sPending(0);
  static int sWrote = 0;
  static unsigned int sQidx = 0;

  if (iAddQ)
    ++sPending;
  while (sPending) {
    if (pthread_mutex_trylock(&sM)) // lock in loop to avoid race condition when sPending==0
      return;
    ThreadQ& aT = sThreads[sQidx];
    if (!pthread_mutex_trylock(&aT.mQchg)) {
      while (aT.mHead) {
        ThreadQ::Qent* aEnt = aT.mHead;
        int aLen = write(sFd, aEnt->buf + sWrote, aEnt->len - sWrote);
        if (aLen < 0) {
          errno != EWOULDBLOCK && errno_exit("ThreadQ::drainQueues write");
          //. race condition if threadloop caller rings now?
          pthread_mutex_unlock(&aT.mQchg);
          pthread_mutex_unlock(&sM);
          epoll_event aEv = { EPOLLIN | EPOLLOUT };
          epoll_ctl(sPollFd, EPOLL_CTL_MOD, sFd, &aEv)
            && errno_exit("ThreadQ::drainQueues epoll_ctl");
          return;
        } else if ((size_t)aLen < aEnt->len - sWrote) {
          sWrote += aLen;
        } else {
          sWrote = 0;
          aT.mHead = aEnt->next;
          free(aEnt);
        }
      }
      if (aT.mTail) {
        aT.mTail = NULL;
        --sPending;
      }
      pthread_mutex_unlock(&aT.mQchg);
    }
    if (++sQidx == sThreadCount)
      sQidx = 0;
    pthread_mutex_unlock(&sM);
  }
}


