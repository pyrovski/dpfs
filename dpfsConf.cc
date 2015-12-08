#include <stdio.h>
#include "SysLock.h"
#include <getopt.h>

using namespace std;

void usage(int argc, char ** argv){
  printf("Usage: %s [--newOSD <storage path>]\n", argv[0]);
}

int main(int argc, char ** argv){

  if(argc == 1){
    usage(argc, argv);
    return 0;
  }
  
  struct option long_options[] = {
    {"newOSD", required_argument, 0, 0}
  };

  int ch;
  bool done = false;

  while(!done){
    int option_index = 0;
    ch = getopt_long(argc, argv, "h", long_options, &option_index);
    switch(ch){
    case -1:
      done = true;
      break;
    case 0:
      printf("option %s\n", long_options[option_index].name);
      if(optarg)
	printf(" with arg %s\n", optarg);
      break;
    case 'h':
    default:
      usage(argc, argv);
      break;
    }
  }
}
