#include <pthread.h>

#ifndef characterEncode
#define characterEncode

#define SPACE ' '
#define NL '\n'
#define FILEEND '~'

#define NUM_CHARS 95

struct binLocator{
  int beginBuffer;
  int beginLoc;
  int endBuffer;
  int endLoc;
};

struct lockedElement{
  int count;
  pthread_mutex_t mutex;
};


void elementInit(struct lockedElement *elem){
  elem->count = 0;
  pthread_mutex_init(&elem->mutex, NULL);
}

#endif
