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

#include "huffmanEncode.h"
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
char **binaryBuffer;
int *binaryCount;
unsigned long int binaryLoc;
unsigned long int binaryChunk;
struct binLocator **locations;

//Changes a string of binary into its ASCII equivalent, using only
//7 bits for each character due to difficulties with characters past
//127
int bitsToChar(char *sBoolText, char *outText) {
    int i,j,len,res;
    int index = 0;

    len = strlen(sBoolText);
    for(i=0;i<len;i+=7) {
        res = 0;
        for(j=0;j<7;j++) {
            res *= 2;
            if (sBoolText[i+j]=='1') res++;
        }
        outText[index++] = res;
    }
    return index;
}


//Thread function to encode a chunk of the input file into its huffman encoded binary
//equivalent, using the index of the thread to calculate where in the file to start
void *binaryEncode(void *_index){
  char **codes = codesPointer[0];
  char* thisFile = (char *)fileIn;
  int threadNum = (int)_index;
  thisFile = thisFile + (chunkSize*threadNum);

  char c;
  for(int i = 0; i < chunkSize; i++){
    c = thisFile[i];
    //Adds the specific character's code to the buffer
    if(validChar(c)){
      int len;
      if(c == NL)
        len = strlen(codes[NUM_CHARS - 1]);
      else
        len = strlen(codes[c-32]);
      for(int i = 0; i < len; i++)
        if(c == NL)
          binaryBuffer[threadNum][binaryCount[threadNum]++] = codes[NUM_CHARS - 1][i];
        else
          binaryBuffer[threadNum][binaryCount[threadNum]++] = codes[c-32][i];
    }
  }
  //Adds the specific character to the file
  if(threadNum == cores - 1){
    int len = strlen(codes[NUM_CHARS]);
    for(int i = 0; i < len; i++)
      binaryBuffer[threadNum][binaryCount[threadNum]++] = codes[NUM_CHARS][i];
  }
}

//Goes through all the binary, building a string of all that the specific thread should
//encode to ASCII characters
void buildBinaryChunk(int threadNum, int currentBuf, int currentLoc, char *thisBinary){

  int done;
  if(currentBuf >= cores)
    done = 0;
  else
    done = 1;
  int index = 0;
  //Adds binary from buffer to this threads chunk
  while(done) {
    thisBinary[index++] = binaryBuffer[currentBuf][currentLoc++];
    if(binaryCount[currentBuf] <= currentLoc){
      currentLoc = 0;
      currentBuf++;
      if(currentBuf >= cores)
        done = 0;
    }
    //Ends once the thread reaches its stopping point
    if((locations[threadNum]->endLoc <=currentLoc && currentBuf == locations[threadNum]->endBuffer))
      done = 0;
  }
}

//Thread function that converts encoded binary into its ASCII equivalent and prints it to the file
void *charEncode(void *_index){

  int threadNum = (int) _index;
  int currentBuf = locations[threadNum]->beginBuffer;
  int currentLoc = locations[threadNum]->beginLoc;

  char* thisBinary = (char *) calloc(binaryChunk, sizeof(char));
  assert(thisBinary != NULL);

  buildBinaryChunk(threadNum, currentBuf, currentLoc, thisBinary);

  char* thisBuffer = (char *) calloc(binaryChunk, sizeof(char));
  assert(thisBuffer != NULL);
  int index;
  index = bitsToChar(thisBinary, thisBuffer);
  free(thisBinary);


  //Sequentially writes encoded characters to the file
  Pthread_mutex_lock(&mutex)
  if(threadNum > 0)
    while(threadDone[threadNum - 1])
      Pthread_cond_wait(&conds[threadNum - 1], &mutex);
  for(int i = 0; i < index; i++)
    fputc(thisBuffer[i], fOut);
  if(threadNum < cores - 1){
    Pthread_cond_signal(&conds[threadNum]);
    threadDone[threadNum] = 0;
  }
  Pthread_mutex_unlock(&mutex);
  free(thisBuffer);
}


//Initializes and memory allocates all data needed for the encoding process
void initializeEncodingData(void *fIn, FILE* fileOut, int inCores, size_t filesize, char **codes){
  fileIn = fIn;
  cores = inCores;
  codesPointer[0] = codes;
  fOut = fileOut;


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

  binaryBuffer = (char **) malloc(cores * sizeof(char *));
  for(int i = 0; i < cores; i++)
    binaryBuffer[i] = (char *) calloc(filesize, sizeof(char));

  binaryCount = (int *)calloc(cores, sizeof(int));
}

//Calculates how many bits each thread must work on in the charEncode function
void calculateThreadChunkForCharEncode(){
  binaryLoc = 0;
  for(int i = 0; i < cores; i++)
    binaryLoc += binaryCount[i];

  if(binaryLoc % 7 == 0)
    binaryChunk = binaryLoc / 7;
  else
    binaryChunk = ((7 - (binaryLoc % 7)) + binaryLoc) / 7;
  if(binaryChunk % cores == 0)
    binaryChunk = binaryChunk / cores;
  else
    binaryChunk = ((cores - (binaryChunk % cores)) + binaryChunk) /cores;


  binaryChunk *= 7;
}

//After binaryEncode, will allocated which binary characters each thread running
//charEncode will work on.  This prevents the need for each thread of binaryEncode
//to have to work sequentially, and allows each charEncode thread to have an
//approximately equal amount of work
void allocateCharEncodeBinary(){

  int prev = 0;
  int leftOver = 0;
  locations = (struct binLocator **) malloc(cores * sizeof(struct binLocator *));

  for(int i = 0; i < cores; i++){
    locations[i] = (struct binLocator *)calloc(1,sizeof(struct binLocation *));
    locations[i]->beginBuffer = prev;
    locations[i]->beginLoc = leftOver;
    if(binaryCount[prev] < binaryChunk + leftOver){
      locations[i]->endBuffer = ++prev;
      leftOver = binaryChunk - binaryCount[prev - 1] + leftOver;
      if(leftOver > binaryCount[prev]){
        leftOver -= binaryCount[prev];
        prev++;
      }
      locations[i]->endLoc = leftOver;
    }else{
      locations[i]->endBuffer = prev;
      leftOver = binaryChunk + leftOver + 1;
      if(leftOver > binaryCount[prev]){
        leftOver = 0;
        prev++;
      }
      locations[i]->endLoc = leftOver;
    }
  }
}

//Prepares all necessary data after the original data is encoded
//to binary, to be ready to be converted to encoded ASCII
void prepareForCharEncode(){


  for(int i = 0; i < NUM_CHARS; i++)
    free(codesPointer[0][i]);
  free(codesPointer[0]);

  calculateThreadChunkForCharEncode();

  allocateCharEncodeBinary();

}

//Frees all final memory allocated data at the end of encode
void freeEncodeData(){
  for(int i = 0; i < cores; i++){
    free(binaryBuffer[i]);
    free(locations[i]);
  }
  free(binaryBuffer);
  free(locations);
  free(binaryCount);


  free(conds);
  free(threadDone);
}


//Given an array of codes, will huffman encode a file, first into the binary
//equivalent defined by the codes, then into the ASCII equivalent of said binary,
//finally printing the final characters into an output file
int encode(void *fIn, FILE* fileOut, int inCores, size_t filesize, char **codes){

  initializeEncodingData(fIn, fileOut, inCores, filesize, codes);

  pthread_t threads[inCores];

  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, binaryEncode, (void *)i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i],NULL);

  prepareForCharEncode();


  for(int i = 0; i < inCores; i++)
    Pthread_create(&threads[i], NULL, charEncode, (void *)i);
  for(int i = 0; i < inCores; i++)
    Pthread_join(threads[i],NULL);

  freeEncodeData();
}
