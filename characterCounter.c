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
int **mappedData;
struct lockedElement **finalLockedCount;
int *finalCount;




int validChar(char c){
  return (c < 126 && c > 31) || c == NL;
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
      if(c == NL)
        index = 94;
      else
        index = (c - 32);
      mappedData[threadNum][index]++;
    }
    thisFile++;
  }
}


void* reduce(void* _index){
  int threadNum = (int) _index;
  for(int i = 0; i < NUM_CHARS; i++){
    int thisFreq = mappedData[threadNum][i];
    Pthread_mutex_lock(&finalLockedCount[i]->mutex);
    finalLockedCount[i]->count += thisFreq;
    Pthread_mutex_unlock(&finalLockedCount[i]->mutex);
  }
}

void makeFinal(){
  for(int i = 0; i < NUM_CHARS; i++)
    finalCount[i] = finalLockedCount[i]->count;
}

void printArray(){
  for(int i = 0; i < NUM_CHARS; i++)
    if(i == 94)
      printf("%i: %c has %i\n", NL, NL,finalCount[i]);
    else
      printf("%i: %c has %i\n", i+32, i+32,finalCount[i]);
}

int* characterCounter(void* fIn, size_t filesize, int inCores, time_t start){

  file = fIn;
  cores = inCores;
  pthread_t threads[inCores];

  if(filesize%inCores == 0)
    chunkSize = filesize/inCores;
  else
    chunkSize = ((inCores - (filesize % inCores)) + filesize) / inCores;

  finalLockedCount = (struct lockedElement *)malloc(NUM_CHARS * sizeof(struct lockedElement *));
  for(int i = 0; i < NUM_CHARS; i++) {
    finalLockedCount[i] = (struct lockedElement *) malloc (sizeof(struct lockedElement));
    elementInit(finalLockedCount[i]);
  }

  mappedData = (int **)malloc(inCores * sizeof(int *));
  for(int i = 0; i < inCores; i++)
    mappedData[i] = (int *) malloc (NUM_CHARS * sizeof(int));

  for(int i = 0; i < NUM_CHARS; i++)
    elementInit(finalLockedCount[i]);


  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, map, (void *)i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i],NULL);

  printf("Map Time: %.2f\n", (double)(time(NULL) - start));


  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, reduce, (void *) i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i], NULL);

  printf("Reduce Time: %.2f\n", (double)(time(NULL) - start));


  finalCount = (int*)malloc(NUM_CHARS * sizeof(int));
  makeFinal();

  for(int i = 0; i < NUM_CHARS; i++)
    free(finalLockedCount[i]);
  free(finalLockedCount);

  for(int i = 0; i < inCores; i++)
    free(mappedData[i]);
  free(mappedData);

  printArray();

  return finalCount;
}
