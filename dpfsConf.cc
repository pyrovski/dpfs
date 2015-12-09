#include <string>
#include <unordered_set>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uuid_s.h"
#include "SysLock.h"
#include "util.h"

using namespace std;

void usage(int argc, char ** argv){
  printf("Usage: %s [--fsid <fsid>] [--newOSD [<storage path>]]\n"
	 "The \"fsid\" option is required if the host has multiple fsids.\n"
	 "Storage path defaults to $HOME/.local/dpfs/OSD/<new OSD UUID>\n", argv[0]);
}

int main(int argc, char ** argv){
  log_t log("/dev/stdout");

  if(argc == 1){
    usage(argc, argv);
    return 0;
  }
  
  struct option long_options[] = {
    {"newOSD", optional_argument, 0, 0},
    {"fsid", required_argument, 0, 'f'},
    {NULL, 0, 0, 0}
  };

  int ch;
  bool done = false;
  bool fsidSpecified = false;
  uuid_s fsid;

  unordered_set<uuid_s> fsids;
  scanFSIDs(log, fsids);

  while(!done){
    int option_index = 0;
    ch = getopt_long(argc, argv, "h", long_options, &option_index);
    const char * option = NULL;
    switch(ch){
    case -1:
      done = true;
      break;
    case 0:
      option = long_options[option_index].name;
      if(!strcmp(option, "newOSD")){
	if(!fsidSpecified){
	  printf("Error: no fsid specified\n");
	  usage(argc, argv);
	  exit(-1);
	}
	return createOSD(log, fsid, optarg);
      } else {
	printf("unknown option: %s\n", option);
      }
      break;
    case 'f':
      //      else if(!strcmp(option, "fsid")){
      if(!uuid_parse(optarg, fsid.uuid)){
	if(fsids.find(fsid) == fsids.end()){
	  printf("FSID not found: %s\n", option);
	  exit(-1);
	}
	fsidSpecified = true;
      } else {
	printf("Invalid fsid: %s", optarg);
	exit(-1);
      }
      break;
    case 'h':
    default:
      usage(argc, argv);
      exit(-1);
    }
  }
}
