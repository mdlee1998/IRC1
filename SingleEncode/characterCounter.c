#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#include "huffmanEncode.h"

char* file;
int *finalCount;

//Checks that a character should be encoded
int validChar(char c){
  return (c < 126 && c > 31) || c == NL;
}


//Prints the counts of each character
void printArray(){
  for(int i = 0; i < NUM_CHARS; i++)
    if(i == 94)
      printf("%i: %c has %i\n", NL, NL,finalCount[i]);
    else
      printf("%i: %c has %i\n", i+32, i+32,finalCount[i]);
}

//Sequentially counts each characters and returns an int array of the counts
int* countCharacters(void* fIn, size_t filesize){

  file = (char *)fIn;
  finalCount = (int*)calloc(NUM_CHARS, sizeof(int));

  char c;
  for(int i = 0; i < filesize; i++){
    c = (char) *file;
    if(validChar(c)){
      if(c == NL)
        finalCount[94]++;
      else
        finalCount[c- 32]++;
    }
    file++;
  }

  printArray();

  return finalCount;
}
