#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"
#include "huffmanTree.c"


struct MinHeapNode* root;
int chunkSize;
int cores;
char *file;
size_t filesize;
FILE* outFile;
pthread_mutex_t mutex;
pthread_cond_t *conds;
int *threadDone;
int ig;

size_t getFilesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

int finishedTreeString(char* buffer){
  for(int i = 0; i < 4; i++)
    if(buffer[i] != '\0')
      return 1;
  return 0;
}

void ascToBinary(unsigned char character, char* buffer, int *idx) {

    if(character == 1)
    {
       buffer[*idx] = '1';
       *idx++;
       return;
    }
    else
    {
        char out;
        if((character%2) == 0)
        {
             out = '0';
             character = character/2;
        }
        else
        {
             out = '1';
             character = character/2;
        }
        ascToBinary(character, buffer, idx);
        buffer[*idx++] = out;
    }
  }

  void ascToBinaryWrapper(char character, char* buffer,int *idx){
    if(character == 0)
      for(int i = 0; i < 8;i++)
        buffer[*idx++] = '0';
    else
      ascToBinary(character, buffer, idx);
  }

int decode (char* binary, struct MinHeapNode* node, char* buffer){
  int length = strlen(binary);
  char c;
  int index = 0;
  for(int i = 0; i < length; i++){
    c = binary[i];
    if(c == '0')
      node = node->left;
    else if(c == '1')
      node = node->right;
    if(isLeaf(node)){
      buffer[index++] = node->data;
      node = root;
    }
  }
  return index;
}

void *threadDecode(void *_index){
  int threadNum = (int)_index;

  Pthread_mutex_lock(&mutex)
  if(threadNum > 0)
    while(threadDone[threadNum - 1])
      Pthread_cond_wait(&conds[threadNum - 1], &mutex);

  unsigned char *thisFile = file += (chunkSize * threadNum);
  char *binary = (char *) malloc(filesize * sizeof(char));
  char *buffer = (char *) malloc((filesize / 4) * sizeof(char));
  int *idx = (int *) malloc(sizeof(int));
  *idx = 0;

  for(ig = 0; ig < chunkSize; ig++)
    ascToBinaryWrapper(thisFile[ig], binary, idx);

  struct MinHeapNode *tmp = root;
  int index = decode(binary, tmp, buffer);
  free(binary);

  Pthread_mutex_lock(&mutex)
  if(threadNum > 0)
    while(threadDone[threadNum - 1])
      Pthread_cond_wait(&conds[threadNum - 1], &mutex);
  for(int i = 0; i < index; i++)
    fputc(buffer[i], outFile);
  if(threadNum < cores - 1){
    Pthread_cond_signal(&conds[threadNum]);
    threadDone[threadNum] = 0;
  }
  Pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[]){

  if (argc != 2){
    printf("Needs a single file\n");
    exit(1);
  }

  filesize= getFilesize(argv[1]);
  int fileNameLen = strlen(argv[1]) + 2;
  cores = get_nprocs();
  pthread_t threads[cores];

  conds = (pthread_cond_t *) malloc ((cores - 1) * sizeof(pthread_cond_t));
  threadDone = (int *) malloc((cores - 1) * sizeof(int));
  for(int i = 0; i < cores - 1; i++) {
    pthread_cond_init(&conds[i], NULL);
    threadDone[i] = 1;
  }

  int fd = open(argv[1], O_RDONLY, 0);
  void* mmappedData = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
  assert(mmappedData != MAP_FAILED);
  file = (char *)mmappedData;

  char *outputFile = (char *) malloc (fileNameLen * sizeof(char));
  strcat(outputFile,"un");
  strcat(outputFile,argv[1]);
  outFile = fopen(outputFile,"w");
  free(outputFile);

  char* treeString = (char *) malloc (255 * 4 * sizeof(char));
  int index = 0;
  int notDone = 1;
  char buffer[4];
  char c;
  while(notDone){
    for(int i = 0; i < 4; i++)
      buffer[i] = (char) file[index+i];
    notDone = finishedTreeString(buffer);
    if(notDone)
      for(int i = 0; i < 4; i++)
        treeString[index++] = buffer[i];
    else
      index+=4;
  }

  root = recreateTree(treeString,0);
  free(treeString);

  file += index;
  filesize-=index;
  if(filesize%cores == 0)
    chunkSize = filesize/cores;
  else
    chunkSize = ((cores - (filesize % cores)) + filesize) / cores;

  for(int i = 0; i < cores; i ++)
    Pthread_create(&threads[i], NULL, threadDecode, (void *)i);
  for(int i = 0; i < cores; i ++)
    Pthread_join(threads[i], NULL);


  return 0;

}
