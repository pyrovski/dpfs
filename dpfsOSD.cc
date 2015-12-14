#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "OSD.h"
#include "defaults.h"
#include "osd.pb.h"
#include "log.h"

using namespace std;

log_t dpfsGlobalLog("/tmp/dpfsOSD.log");

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
  OSD osd(defaultOSDPort);
  
  globalOSD = &osd;

  return osd.run(foreground);
}
