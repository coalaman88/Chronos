#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int arg, char* argv[]){
  if(arg <= 1){
    puts("no args!");
    return 0;
  }

  // parse int arg
  int read_int;
  sscanf(argv[1], "%d", &read_int);
  assert(read_int >= 0);

  printf("waiting %d ms...\n", read_int);
  Sleep(read_int);
  puts("end!");
}
