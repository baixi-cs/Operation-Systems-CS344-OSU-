/*
AUTHOR: Joshua Strozzi
PROGRAM: keygen for otp
CLASS: CS-344
DUE: NOV 11 - 11:59PM
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argv, char* argc[]){
  srand(time(0));

  if(argv < 2){
    perror("not enough arguments\n");
    return 1;
  }

  int numlen = strlen(argc[1]);
  int keyNum = 0, it=0, factor=1, result =0 ;

  for(it = 0; it<numlen-1; it++){
    factor *= 10;
  }
  for(it = 0; it<numlen; it++){
    result += (argc[1][it] - '0') * factor;
    factor = factor/10;
  }
  keyNum = result;
  //printf("result: %d\n", result);


  char* Key = malloc(result*sizeof(char));
  char currLetter;

  for(it =0; it<result; it++){
    currLetter = ('A' +rand()%27 );
    if(currLetter == '[')
      currLetter = ' ';
    Key[it]= currLetter;
  }

  printf("%s\n", Key);

  free(Key);
  return 0;
}
