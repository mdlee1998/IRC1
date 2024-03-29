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
#include "huffmanTree.c"
#include "encode.c"


size_t getFilesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

int main(int argc, char** argv) {
    size_t filesize = getFilesize(argv[1]);
    int fd = open(argv[1], O_RDONLY, 0);
    assert(fd != -1);

    void* mmappedData = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    assert(mmappedData != MAP_FAILED);

    int *freqs;

    freqs = countCharacters(mmappedData, filesize);


    char** codes = (char **) malloc (NUM_CHARS * sizeof(char*));
    for(int i = 0; i < NUM_CHARS; i++)
      codes[i] = (char *) malloc (20 * sizeof(char));

    char *treeString = (char *) malloc (255 * 4 * sizeof(char));

    int fileChars = 0;
    for(int i = 0; i < NUM_CHARS; i++)
      if(freqs[i] > 0)
        fileChars++;

    int nodeCount = createHuffmanTree(freqs, fileChars, codes, treeString);

    free(freqs);


    int fileLength = strlen(argv[1]) + 6;
    char *outputFile = (char *) calloc (fileLength, sizeof(char));
    strcat(outputFile,"zipped");
    char* fileEnd = strrchr(argv[1],'/');
    if(fileEnd != NULL){
      fileEnd[1] = toupper(fileEnd[1]);
      strcat(outputFile,&fileEnd[1]);
    }
    else{
      argv[1][0] = toupper(argv[1][0]);
      strcat(outputFile,argv[1]);
    }
    FILE* outFile = fopen(outputFile,"w");
    assert(outFile != NULL);
    free(outputFile);

    fwrite((void *)treeString,1,nodeCount,outFile);
    fwrite((void *)"aa",1,2,outFile);
    free(treeString);

    encode(mmappedData, outFile, filesize, codes);



    assert (munmap(mmappedData, filesize) == 0);
    close(fd);
    return 0;
}
