#ifndef DPFS_HH
#define DPFS_HH

typedef enum {
  dpfs_fuse_getattr,
  dpfs_fuse_readdir,
  dpfs_fuse_open,
  dpfs_fuse_read,
  dpfs_fuse_write,
  dpfs_fuse_mkdir,
  dpfs_fuse_unlink,
  dpfs_fuse_truncate
} dpfs_fuse_op;

const char * dpfs_fuse_opnames[] = {
  "getattr",
  "readdir",
  "open",
  "read",
  "write",
  "mkdir",
  "unlink",
  "truncate"
};

#endif
