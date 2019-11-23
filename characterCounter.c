#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#include "huffmanEncode.h"
#include "common.h"
#include "common_threads.h"

void* file;
size_t chunkSize;
int cores;
int **mappedData;
struct lockedElement **finalLockedCount;
int *finalCount;

//Initializes file, number of cores, pthread array, chunkSize
//locked counter, and counter arrays
void initializeCharacterCounter(void* fIn, size_t filesize, int inCores){
  file = fIn;
  cores = inCores;

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
}

//Checks that a given character is encodable
int validChar(char c){
  return (c < 126 && c > 31) || c == NL;
}

//Given the index of the specific thread running this function
//will determine where in the input file to start then run through
//one thread chunk counting the frequencies of each character and
//storing them into mappedData array
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
      //Index for newlines
      if(c == NL)
        index = 94;
      //Index for all other characters
      else
        index = (c - 32);
      //Add to this threads count array
      mappedData[threadNum][index]++;
    }
    //Iterate through file
    thisFile++;
  }
}

//Goes through one thread's mappedData and adds each
//count to a final, shared,  mutex locked counter
void* reduce(void* _index){
  int threadNum = (int) _index;
  for(int i = 0; i < NUM_CHARS; i++){
    int thisFreq = mappedData[threadNum][i];
    //Locks the specific element of the list and adds to it
    Pthread_mutex_lock(&finalLockedCount[i]->mutex);
    finalLockedCount[i]->count += thisFreq;
    Pthread_mutex_unlock(&finalLockedCount[i]->mutex);
  }
}

//Coverts the mutex locked shared count to an int array
void makeFinal(){
  for(int i = 0; i < NUM_CHARS; i++)
    finalCount[i] = finalLockedCount[i]->count;
}

//Prints the counts of each character
void printArray(){
  for(int i = 0; i < NUM_CHARS; i++)
    if(i == 94)
      printf("%i: %c has %i\n", NL, NL,finalCount[i]);
    else
      printf("%i: %c has %i\n", i+32, i+32,finalCount[i]);
}

//Frees memory allocated data at the end of the character counter
void freeCharacterCounterData(){
  for(int i = 0; i < NUM_CHARS; i++)
    free(finalLockedCount[i]);
  free(finalLockedCount);

  for(int i = 0; i < cores; i++)
    free(mappedData[i]);
  free(mappedData);
}

//Given a file, its size and a number of threads, will count all
//the occurences of each character in parallel, and return an int
//array of these counts
int* characterCounter(void* fIn, size_t filesize, int inCores){

  initializeCharacterCounter(fIn, filesize, inCores);
  pthread_t threads[inCores];

  //Map
  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, map, (void *)i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i],NULL);

  //Reduce
  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, reduce, (void *) i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i], NULL);

  //Reverts the array of mutex locked ints into an array of ints
  finalCount = (int*)malloc(NUM_CHARS * sizeof(int));
  makeFinal();

  freeCharacterCounterData();

  printArray();

  return finalCount;
}
