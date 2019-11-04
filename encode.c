#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <pthread.h>

#include "characterLocations.h"
#include "huffmanTree.c"
#include "common.h"
#include "common_threads.h"

void *fileIn;
size_t chunkSize;
int cores;
char **codesPointer[1];
FILE *fOut;
pthread_mutex_t mutex;
pthread_cond_t *conds;
int *threadDone;
char *binaryBuffer;
unsigned long int binaryLoc;
unsigned long int binaryChunk;

int bitsToChar(char *sBoolText, char *outText) {
    int i,j,len,res;
    int index = 0;

    len = strlen(sBoolText);
    for(i=0;i<len;i+=7) {
        res = 0;
        // build one byte with 8bits as characters
        for(j=0;j<7;j++) {
            res *= 2;
            if (sBoolText[i+j]=='1') res++;
        }
        outText[index++] = res;
    }
    return index;
}

void *binaryEncode(void *_index){
  char **codes = codesPointer[0];
  char* thisFile = (char *)fileIn;
  int threadNum = (int)_index;
  thisFile = thisFile + (chunkSize*threadNum);

  unsigned char *buffer = (char *) malloc (chunkSize * 8 * sizeof(char));
  assert(buffer != NULL);


  char c;
  int bufferIndex = 0;
  for(int i = 0; i < chunkSize; i++){
    c = thisFile[i];
    if(validChar(c)){
      int len;
      if(c == NL)
        len = strlen(codes[94]);
      else
        len = strlen(codes[c-32]);
      for(int i = 0; i < len; i++)
        if(c == NL)
          buffer[bufferIndex++] = codes[94][i];
        else
          buffer[bufferIndex++] = codes[c-32][i];
    }
  }

  realloc(buffer, bufferIndex);

  Pthread_mutex_lock(&mutex)
  if(threadNum > 0)
    while(threadDone[threadNum - 1])
      Pthread_cond_wait(&conds[threadNum - 1], &mutex);
  for(int i = 0; i < bufferIndex; i++)
    binaryBuffer[binaryLoc++] = buffer[i];
  if(threadNum < cores - 1){
    Pthread_cond_signal(&conds[threadNum]);
    threadDone[threadNum] = 0;
  }
  Pthread_mutex_unlock(&mutex);

  free(buffer);
}

void *charEncode(void *_index){

  int threadNum = (int) _index;
  unsigned long int start = threadNum * binaryChunk;
  unsigned long int end = (binaryChunk/7) + 6;

  char* thisBinary = (char *) calloc(binaryChunk, sizeof(char));
  assert(thisBinary != NULL);
  char* thisBuffer = (char *) calloc(end, sizeof(char));
  assert(thisBuffer != NULL);

  for(int i = 0; i < binaryChunk; i++)
    thisBinary[i] = binaryBuffer[start++];

  int index = bitsToChar(thisBinary, thisBuffer);
  free(thisBinary);



  Pthread_mutex_lock(&mutex)
  if(threadNum > 0)
    while(!threadDone[threadNum - 1])
      Pthread_cond_wait(&conds[threadNum - 1], &mutex);
  for(int i = 0; i < index; i++)
    fputc(thisBuffer[i], fOut);
  if(threadNum < cores - 1){
    Pthread_cond_signal(&conds[threadNum]);
    threadDone[threadNum] = 1;
  }
  Pthread_mutex_unlock(&mutex);
  free(thisBuffer);
}

int encode(void *fIn, FILE* fileOut, int inCores, size_t filesize, char **codes, time_t start){
  fileIn = fIn;
  cores = inCores;
  codesPointer[0] = codes;
  fOut = fileOut;

  pthread_t threads[inCores];
  conds = (pthread_cond_t *) malloc ((inCores - 1) * sizeof(pthread_cond_t));
  threadDone = (int *) malloc((inCores - 1) * sizeof(int));
  for(int i = 0; i < inCores - 1; i++) {
    pthread_cond_init(&conds[i], NULL);
    threadDone[i] = 1;
  }

  if(filesize%inCores == 0)
    chunkSize = filesize/inCores;
  else
    chunkSize = ((inCores - (filesize % inCores)) + filesize) / inCores;

  binaryBuffer = (char *) calloc(filesize * 8, sizeof(char));
  binaryLoc = 0;

  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, binaryEncode, (void *)i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i],NULL);

  printf("Binary Encode: %.2f\n", (double)(time(NULL) - start));

  realloc(binaryBuffer, binaryLoc);

  for(int i = 0; i < NUM_CHARS; i++)
    free(codes[i]);
  free(codes);

  if(binaryLoc % cores == 0)
    binaryChunk = binaryLoc/cores;
  else
    binaryChunk = ((cores - (binaryLoc % cores)) + binaryLoc) / cores;

  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, charEncode, (void *)i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i],NULL);

  printf("Char Encode: %.2f\n", (double)(time(NULL) - start));


  free(binaryBuffer);
  free(conds);
  free(threadDone);
}
