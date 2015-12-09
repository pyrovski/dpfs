#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "OSD.h"
#include "defaults.h"
#include "osd.pb.h"

using namespace std;

OSD *globalOSD = NULL;

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

  //!@todo change for multiple OSDs per host
  OSD osd(defaultOSDPort, "/tmp/dpfsOSD.log");
  
  globalOSD = &osd;

  return osd.run(foreground);
}
