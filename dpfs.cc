#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static const int notImplemented = -EOPNOTSUPP;

static int dpfs_getattr(const char * path, struct stat * result_stat){
  return notImplemented;
}

static int dpfs_readdir(const char * path, void * buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info * file_info){
  return notImplemented;
}

static int dpfs_open(const char * path, struct fuse_file_info * file_info){
  return notImplemented;
}

static int dpfs_read(const char * path, char * buf, size_t size, off_t offset,
		     struct fuse_file_info * file_info){
  return notImplemented;
}

static int dpfs_write(const char * path, const char * buf, size_t size,
		      off_t offset, struct fuse_file_info * file_info){
  return notImplemented;
}

static int dpfs_mkdir(const char * path, mode_t mode){
  return notImplemented;
}

static int dpfs_unlink(const char * path){
  return notImplemented;
}

static int dpfs_truncate(const char * path, off_t offset){
  return notImplemented;
}

static struct fuse_operations fuse_oper;

int main(int argc, char ** argv){

  fuse_oper.getattr = dpfs_getattr;
  fuse_oper.readdir = dpfs_readdir;
  fuse_oper.open = dpfs_open;
  fuse_oper.read = dpfs_read;
  fuse_oper.write = dpfs_write;
  fuse_oper.mkdir = dpfs_mkdir;
  fuse_oper.unlink = dpfs_unlink;
  fuse_oper.truncate = dpfs_truncate;
  
  return fuse_main(argc, argv, &fuse_oper, NULL);
}
