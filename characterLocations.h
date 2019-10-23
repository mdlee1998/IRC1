#include <pthread.h>

#ifndef characterLocations
#define characterLocations

#define SPACE ' '
#define NL '\n'

#define QUESTION '?'
#define PERIOD '.'
#define EXCLAMATION '!'
#define DOUBLEQUO '\"'
#define SINQUOTE '\''
#define COMMA ','
#define OPENPAREN '('
#define CLOSEPAREN ')'

#define NUM_CHARS 94

struct lockedElement{
  int count;
  pthread_mutex_t mutex;
};


void elementInit(struct lockedElement *elem){
  elem->count = 0;
  pthread_mutex_init(&elem->mutex, NULL);
}

#endif
