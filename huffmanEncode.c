#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <time.h>

#include "huffmanEncode.h"
#include "characterCounter.c"
#include "encode.c"

int cores;
size_t filesize;
void* mmappedData;
int fd;
FILE* outFile;
char** codes;
char *treeString;

//Gets size of a file
size_t getFilesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

//Creates a new filename for the compressed file, by
//adding zipped to the original files name.  Capitalizes
//first character of the original name and gets rid of path
void createOutFile(char *inFile){
  int fileLength = strlen(inFile) + 6;
  char *outputFile = (char *) calloc (fileLength, sizeof(char));
  strcat(outputFile,"zipped");
  //Gets rid of path
  char* fileEnd = strrchr(inFile,'/');
  //Makes first char uppercase and add zipped to beginning
  if(fileEnd != NULL){
    fileEnd[1] = toupper(fileEnd[1]);
    strcat(outputFile,&fileEnd[1]);
  }
  else{
    inFile[0] = toupper(inFile[0]);
    strcat(outputFile,inFile);
  }
  outFile = fopen(outputFile,"w");
  assert(outFile != NULL);
}

//Initializes all data needed and creates a huffman tree,Initializes basic data for the encoding process, including
//mapping the data and
//returning the number of nodes in the tree
int initializeHuffmanTree(int *freqs){

      codes = (char **) malloc ((NUM_CHARS + 1) * sizeof(char*));
      for(int i = 0; i <= NUM_CHARS; i++)
        codes[i] = (char *) malloc (20 * sizeof(char));

      treeString = (char *) malloc (255 * 4 * sizeof(char));

      int fileChars = 0;
      for(int i = 0; i < NUM_CHARS; i++)
        if(freqs[i] > 0)
          fileChars++;

      int nodeCount = createHuffmanTree(freqs, fileChars + 1, codes, treeString);
      free(freqs);
      return nodeCount;
}

//Maps the data, sets the number of threads to use and
//finds the filesize.
void initializeData(char *inFile){
  cores = get_nprocs();
  filesize = getFilesize(inFile);
  fd = open(inFile, O_RDONLY, 0);
  assert(fd != -1);

  mmappedData = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
  assert(mmappedData != MAP_FAILED);
}

//Closes the file and unmaps the data
void closeFiles(){
  assert (munmap(mmappedData, filesize) == 0);
  close(fd);
}

//Parallel huffman encodes any file given
int main(int argc, char** argv) {
    initializeData(argv[1]);

    int *freqs;
    freqs = characterCounter(mmappedData, filesize, cores);

    int nodeCount = initializeHuffmanTree(freqs);

    createOutFile(argv[1]);

    //Writes the treestring to the beginning of the encoded file for
    //rebuilding the tree.  Two lowercase a's are added to represent
    //the end of the tree
    fwrite((void *)treeString,1,nodeCount,outFile);
    fwrite((void *)"aa",1,2,outFile);
    int treeLength = strlen(treeString) + 2;
    free(treeString);

    encode(mmappedData, outFile, cores, filesize, codes);

    closeFiles();
    return 0;
}
