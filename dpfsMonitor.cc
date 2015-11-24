#include <stdio.h>
#include <unistd.h>
#include "monitor.h"
#include "defaults.h"

void usage(const char * name){
  fprintf(stderr, "usage: %s [-f]\n-f: foreground\n", name);
}

int main(int argc, char ** argv){
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  bool foreground = false;
  
  int status;
  while((status = getopt(argc, argv, "f")) != -1){
    switch(status){
    case 'f':
      foreground = true;
      break;
    default:
      usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  
  monitor mon(defaultMonPort, "/tmp/dpfsMon.log");
  return mon.run(foreground);
}
