#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <time.h>

#include "characterLocations.h"
#include "characterCounter.c"
#include "encode.c"


size_t getFilesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

int main(int argc, char** argv) {
    time_t start = time(NULL);
    int cores = get_nprocs();
    size_t filesize = getFilesize(argv[1]);
    int fd = open(argv[1], O_RDONLY, 0);
    assert(fd != -1);

    void* mmappedData = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    assert(mmappedData != MAP_FAILED);

    int *freqs;
    freqs = characterCounter(mmappedData, filesize, cores);

    char** codes = (char **) malloc (NUM_CHARS * sizeof(char*));
    for(int i = 0; i < NUM_CHARS; i++)
      codes[i] = (char *) malloc (20 * sizeof(char));

    char *treeString = (char *) malloc (255 * 4 * sizeof(char));

    int nodeCount = createHuffmanTree(freqs, NUM_CHARS, codes, treeString);

    int fileLength = strlen(argv[1]) + 6;
    char *outputFile = (char *) malloc (fileLength * sizeof(char));
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

    fwrite((void *)treeString,1,nodeCount * 4,outFile);
    fwrite((void *)"\0\0\0\0",1,4,outFile);
    free(treeString);

    encode(mmappedData, outFile, cores, filesize, codes);

    for(int i = 0; i < NUM_CHARS; i++)
      free(codes[i]);
    free(codes);

    assert (munmap(mmappedData, filesize) == 0);
    close(fd);
    printf("%.2f\n", (double)(time(NULL) - start));
    exit(0);
}
