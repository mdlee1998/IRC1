#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#include "characterLocations.h"

void* file;
size_t chunkSize;
int cores;
int reduceChars;
int** arrayPointer[1];
int finalCount[NUM_CHARS] = {0};
char charArray[NUM_CHARS] = {SPACE, EXCLAMATION, DOUBLEQUO, SINQUOTE, OPENPAREN, CLOSEPAREN, COMMA, PERIOD, '0', '1', '2', '3', '4' ,'5', '6', '7', '8', '9', QUESTION, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};




int validChar(char c){
  return isalnum(c) || c == SPACE || c == NL || c == PERIOD || c == QUESTION || c == EXCLAMATION || c == DOUBLEQUO || c == SINQUOTE || c == COMMA || c == OPENPAREN || c == CLOSEPAREN ;
}

void* map(void* _index){
  char* thisFile = (char *)file;
  int index = (int)_index;
  thisFile = thisFile + (chunkSize*index);

  int *freq = arrayPointer[0][index];
  int charNum;
  for(int i = 0; i < chunkSize; i++){
    char c = *thisFile;
    int arrayIndex;
    if(validChar(c)){
      if(c == NL)
        arrayIndex = 0;
      else if(c == SPACE)
        arrayIndex = 1;
      else if(c == EXCLAMATION)
        arrayIndex = 2;
      else if(c == DOUBLEQUO)
        arrayIndex = 3;
      else if(c == SINQUOTE)
        arrayIndex = 4;
      else if(c == OPENPAREN)
        arrayIndex = 5;
      else if(c == CLOSEPAREN)
        arrayIndex = 6;
      else if(c == COMMA)
        arrayIndex = 7;
      else if(c == PERIOD)
        arrayIndex = 8;
      else if(isdigit(c))
        arrayIndex = c - 39;
      else if(c == QUESTION)
        arrayIndex = 19;
      else if(isupper(c))
        arrayIndex = c-45;
      else if(islower(c))
        arrayIndex = c-51;
      freq[arrayIndex]++;
    }
    thisFile++;
  }
}


void* reduce(void* _index){
  int index = (int) _index;
  int start = index * reduceChars;
  int end = start + reduceChars;
  for(int i = 0; i < cores; i++)
    for(start = index * reduceChars; start < end; start++)
      finalCount[start] += arrayPointer[0][i][start];
}

void printArray(){
  for(int i = 0; i < NUM_CHARS; i++)
    printf("%c has %i\n",charArray[i],finalCount[i]);
}

int** characterCounter(void* fIn, size_t filesize, int inCores){
  file = fIn;
  cores = inCores;
  if(NUM_CHARS % inCores == 0)
    reduceChars = NUM_CHARS / inCores;
  else
    reduceChars = ((inCores - (NUM_CHARS % inCores)) + NUM_CHARS) / inCores;
  pthread_t threads[inCores];
  if(filesize%inCores == 0)
    chunkSize = filesize/inCores;
  else
    chunkSize = ((inCores - (filesize % inCores)) + filesize) / inCores;
  int** array = (int**)malloc(inCores * sizeof(int *));
  for(int i = 0; i < inCores; i++)
    array[i] = (int *)malloc(NUM_CHARS * sizeof(int));
  arrayPointer[0] = array;
  for(int i = 0; i < inCores; i++)
    pthread_create(&threads[i], NULL, map, (void *)i);
  for(int i = 0; i < inCores; i++)
    pthread_join(threads[i],NULL);
  for(int i = 0; i < inCores; i++)
    pthread_create(&threads[i], NULL, reduce, (void *) i);
  for(int i = 0; i < inCores; i++)
    pthread_join(threads[i], NULL);

  printArray();
  free(array);

  return &finalCount;
}
