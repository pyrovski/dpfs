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
  printf("\nUsage:\n"
	 "%s [--fsid <fsid>] [--newOSD [<storage path>]]\n"
	 //"The \"fsid\" option is required if the host has multiple fsids.\n"
	 "Storage path defaults to $HOME/.local/dpfs/OSD/<new OSD UUID>\n"
	 "\n"
	 "%s [--newFS --pgs <# of PGs>]\n"
	 "\n",
	 argv[0], argv[0]);
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
    {"newFS", no_argument, 0, 0},
    {"pgs", required_argument, 0, 'p'},
    {NULL, 0, 0, 0}
  };

  int ch;
  int status;
  bool done = false;
  bool fsidSpecified = false;
  bool newFS = false;
  uuid_s fsid;
  char fsidStr[37];
  FSOptions::FSOptions fsOptions;
  char * nextChar = NULL;

  uuid_clear(fsid.uuid);
  
  unordered_set<uuid_s> fsids;
  scanFSIDs(log, fsids);

  while(!done){
    int option_index = 0;
    ch = getopt_long(argc, argv, "hp", long_options, &option_index);
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
	  exit(EXIT_FAILURE);
	}
	return createOSD(log, fsid, optarg);
      } else if(!strcmp(option, "newFS")){
	newFS = true;
      } else {
	printf("unknown option: %s\n", option);
      }
      break;
    case 'f':
      //      else if(!strcmp(option, "fsid")){
      if(!uuid_parse(optarg, fsid.uuid)){
	if(fsids.find(fsid) == fsids.end()){
	  printf("FSID not found: %s\n", option);
	  exit(EXIT_FAILURE);
	}
	fsidSpecified = true;
      } else {
	printf("Invalid fsid: %s", optarg);
	exit(EXIT_FAILURE);
      }
      break;
    case 'p':
      errno = 0;
      status = strtol(optarg, &nextChar, 10);
      if(errno || nextChar == optarg){
	printf("Invalid number of PGs");
	exit(EXIT_FAILURE);
      }
      fsOptions.set_pgs(status);
      break;
    case 'h':
    default:
      usage(argc, argv);
      exit(EXIT_FAILURE);
    }
  } // while

  if(newFS){
    status = createFS(log, fsid, fsOptions);
    if(!status){
      uuid_unparse(fsid.uuid, fsidStr);
      printf("%s\n", fsidStr);
    } else {
      printf("Failed to create new FS: %d\n", status);
	  exit(EXIT_FAILURE);
    }
  }
}
