#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "Monitor.h"
#include "defaults.h"
#include "mon.pb.h"

using namespace std;

Monitor *globalMonitor = NULL;

void usage(const char * name){
  fprintf(stderr, "usage: %s [-f]\n-f: foreground\n", name);
}

void handler(int signal){
  if(!globalMonitor)
    return;

  globalMonitor->quit();
}

int installSignalHandler(){
  struct sigaction sa = {};
  sa.sa_handler = &handler;
  int status = sigaction(SIGINT, &sa, NULL);
  if(status){
    cerr << "failed to install signal handler: " << strerror(errno) << endl;
    return -1;
  }
  return 0;
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

  installSignalHandler();
  
  Monitor mon(defaultMonPort, "/tmp/dpfsMon.log");
  globalMonitor = &mon;

  return mon.run(foreground);
}
