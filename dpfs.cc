//!@todo signal handler

#include <thread>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>

#include "clientCache.h"
#include "log.h"
#include "dpfs.h"
#include "osd.h"
#include "MonManager.h"
#include "defaults.h"
#include "Conf.h"

using namespace std;

static const int notImplemented = -EOPNOTSUPP;

static clientCache cache;
log_t dpfsGlobalLog("/tmp/dpfs.log");
static Conf conf;
static MonManager * monManager;

static int defaultAction(const char * path, int op){
  logmsg(dpfs_fuse_opnames[op]);
  return notImplemented;
}

static int dpfs_getattr(const char * path, struct stat * result_stat){
  //!@todo depends on having some representation of our FS metadata...
  return defaultAction(path, dpfs_fuse_getattr);
}

static int dpfs_readdir(const char * path, void * buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info * file_info){
  return defaultAction(path, dpfs_fuse_readdir);
}

static int dpfs_open(const char * path, struct fuse_file_info * file_info){
  return defaultAction(path, dpfs_fuse_open);
}

static int dpfs_read(const char * path, char * buf, size_t size, off_t offset,
		     struct fuse_file_info * file_info){
  return defaultAction(path, dpfs_fuse_read);
}

static int dpfs_write(const char * path, const char * buf, size_t size,
		      off_t offset, struct fuse_file_info * file_info){
  return defaultAction(path, dpfs_fuse_write);
}

static int dpfs_mkdir(const char * path, mode_t mode){
  return defaultAction(path, dpfs_fuse_mkdir);
}

static int dpfs_unlink(const char * path){
  return defaultAction(path, dpfs_fuse_unlink);
}

static int dpfs_truncate(const char * path, off_t offset){
  return defaultAction(path, dpfs_fuse_truncate);
}

static struct fuse_operations fuse_oper;

int main(int argc, char ** argv){
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  int status;

  fuse_oper.getattr = dpfs_getattr;
  fuse_oper.readdir = dpfs_readdir;
  fuse_oper.open = dpfs_open;
  fuse_oper.read = dpfs_read;
  fuse_oper.write = dpfs_write;
  fuse_oper.mkdir = dpfs_mkdir;
  fuse_oper.unlink = dpfs_unlink;
  fuse_oper.truncate = dpfs_truncate;

  conf.load();
  if(!conf.hasKey("fsid")){
    //!@todo merge fsid and other options from command line and conf
    failmsg("FSID required.");
  }
  uuid_t fsid;
  uuid_parse(conf.get("fsid")->c_str(), fsid);
  monManager = new MonManager(conf.get("monitors"), fsid);
  monManager->start();

  dbgmsg("waiting for fsid from monitor");
  status = monManager->validateFSID();
  dbgmsg("got fsid from monitor");
  
  status = fuse_main(argc, argv, &fuse_oper, NULL);
  dbgmsg("fuse_main finished");
  monManager->stop();
  return status;
}
