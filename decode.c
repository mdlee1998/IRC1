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
struct MinHeapNode* cur;
int chunkSize;
int cores;
char *file;
size_t filesize;
FILE* outFile;
pthread_mutex_t mutex;
pthread_cond_t *conds;
int *threadDone;


size_t getFilesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}


void ascToBinary(unsigned char character, char* buffer, int *idx) {

    for(int i = 7; i > 0; i--){
      if((int)character % 2 == 1)
        buffer[*idx + i - 1] = '1';
      else
        buffer[*idx + i - 1] = '0';
      character /= 2;
    }
    *idx = *idx + 7;
  }


int decode (char* binary, char* buffer){
  int length = strlen(binary);
  char c;
  int index = 0;
  int count = 0;
  for(int i = 0; i < length; i++){
    c = binary[i];
    if(c == '0')
      cur = cur->left;
    else if(c == '1')
      cur = cur->right;
    if(isLeaf(cur)){
      buffer[index++] = cur->data;
      cur = root;
    }
  }
  return index;
}

void *threadDecode(void *_index){
  int threadNum = (int)_index;

  unsigned char *thisFile = file + (chunkSize * threadNum);
  char *binary = (char *) malloc(filesize * sizeof(char));
  char *buffer = (char *) malloc((filesize / 4) * sizeof(char));

  int *idx = (int *) malloc(sizeof(int));
  *idx = 0;
  char c;
  for(int i = 0; i < chunkSize; i++){
    c = thisFile[i];
    if(c == '\\')
      if(thisFile[i+1] == '0')
        if(thisFile[i+2] == '0')
          c = '\0';
    ascToBinary(c, binary, idx);
  }

  free(idx);

  Pthread_mutex_lock(&mutex)
  if(threadNum > 0)
    while(threadDone[threadNum - 1])
      Pthread_cond_wait(&conds[threadNum - 1], &mutex);
  int index = decode(binary, buffer);
  free(binary);
  for(int i = 0; i < index; i++)
    fputc(buffer[i], outFile);
  if(threadNum < cores - 1){
    Pthread_cond_signal(&conds[threadNum]);
    threadDone[threadNum] = 0;
  }
  Pthread_mutex_unlock(&mutex);
  free(buffer);
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
  char c;

  while(notDone) {
    c = (char) file[index];
    if(c == 'a')
      if(file[index + 1] == 'a')
        notDone = 0;
    if(notDone)
      treeString[index++] = c;
  }

  root = recreateTree(treeString,index + 1, index);
  free(treeString);

  cur = root;


  file += index + 2;
  filesize-=index - 2;

  if(filesize%cores == 0)
    chunkSize = filesize/cores;
  else
    chunkSize = ((cores - (filesize % cores)) + filesize) / cores;

  for(int i = 0; i < cores; i ++)
    Pthread_create(&threads[i], NULL, threadDecode, (void *)i);
  for(int i = 0; i < cores; i ++)
    Pthread_join(threads[i], NULL);

  freeHuffmanTree(root);
  free(conds);

  free(threadDone);
  free(root);

  assert (munmap(mmappedData, filesize) == 0);
  close(fd);

  return 0;

}
