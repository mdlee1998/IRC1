#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <time.h>

#include "characterCounter.c"

char *fileIn;
char **codesPointer[1];
FILE *fOut;
char *binaryBuffer;
size_t filesize;

//Converts a string of bits into characters and writes them to the output file
int bitsToChar(char *sBoolText) {
    int j,res;
    unsigned long int len, i;
    int index = 0;

    len = strlen(sBoolText);
    for(i=0;i<len;i+=7) {
        res = 0;
        // build one byte with 8bits as characters
        for(j=0;j<7;j++) {
            res *= 2;
            if (sBoolText[i+j]=='1') res++;
        }
        fputc(res, fOut);
    }
    return index;
}

//Encodes the file into the Huffman Encoded binary
void *binaryEncode(){
  char **codes = codesPointer[0];
  char c;
  unsigned long int bufferIndex = 0;
  for(int i = 0; i < filesize; i++){
    c = fileIn[i];
    if(validChar(c)){
      int len;
      if(c == NL)
        len = strlen(codes[94]);
      else
        len = strlen(codes[c-32]);
      for(int i = 0; i < len; i++)
        if(c == NL)
          binaryBuffer[bufferIndex++] = codes[94][i];
        else
          binaryBuffer[bufferIndex++] = codes[c-32][i];
    }
  }
}

//Given a file and a set of codes, will compress the file using the codes to
//replace the characters
int encode(void *fIn, FILE* fileOut, size_t fsize, char **codes){
  fileIn = (char *) fIn;
  codesPointer[0] = codes;
  fOut = fileOut;
  filesize = fsize;

  binaryBuffer = (char *) calloc(filesize * 8, sizeof(char));

  binaryEncode();


  for(int i = 0; i < NUM_CHARS; i++)
    free(codes[i]);
  free(codes);

  bitsToChar(binaryBuffer);

  free(binaryBuffer);
}
