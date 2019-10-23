#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#include "characterLocations.h"
#include "common.h"
#include "common_threads.h"

void* file;
size_t chunkSize;
int cores;
int *mappedData;
struct lockedElement *finalLockedCount;
int *finalCount;




int validChar(char c){
  return c < 127 && c > 31;
}

void* map(void* _index){
  char* thisFile = (char *)file;
  int threadNum = (int)_index;
  int offset = threadNum * NUM_CHARS;
  thisFile = thisFile + (chunkSize*threadNum);
  int charNum;
  for(int i = 0; i < chunkSize; i++){
    char c = *thisFile;
    int index;
    if(validChar(c)){
      index = (c - 32) + offset;
      mappedData[index]++;
    }
    thisFile++;
  }
}


void* reduce(void* _index){
  int index = (int) _index;
  int start = index * NUM_CHARS;
  int end = start + NUM_CHARS;
  for(; start < end; start++){
    int thisFreq = mappedData[start];
    Pthread_mutex_lock(&finalLockedCount[start % NUM_CHARS].mutex);
    finalLockedCount[start % NUM_CHARS].count += thisFreq;
    Pthread_mutex_unlock(&finalLockedCount[start % NUM_CHARS].mutex);
  }
}

void makeFinal(){
  for(int i = 0; i < NUM_CHARS; i++)
    finalCount[i] = finalLockedCount[i].count;
}

void printArray(){
  for(int i = 0; i < NUM_CHARS; i++)
    printf("%c has %i\n",i+32,finalCount[i]);
}

int* characterCounter(void* fIn, size_t filesize, int inCores){
  file = fIn;
  cores = inCores;
  pthread_t threads[inCores];
  if(filesize%inCores == 0)
    chunkSize = filesize/inCores;
  else
    chunkSize = ((inCores - (filesize % inCores)) + filesize) / inCores;
  finalLockedCount = (struct lockedElement*)malloc(NUM_CHARS * sizeof(struct lockedElement));
  mappedData = (int*)malloc(inCores * NUM_CHARS * sizeof(int));
  finalCount = (int*)malloc(NUM_CHARS * sizeof(int));
  for(int i = 0; i < NUM_CHARS; i++)
    elementInit(&finalLockedCount[i]);


  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, map, (void *)i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i],NULL);
  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, reduce, (void *) i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i], NULL);

  makeFinal();
  printArray();
  free(finalLockedCount);
  //free(mappedData);
  return finalCount;
}
