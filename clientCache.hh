#ifndef CLIENTCACHE_HH
#define CLIENTCACHE_HH

#include <string>
#include <stdint.h>

#include <unordered_map>
#include <queue>

#include "writeRequest.hh"

/*
  What is the format of a request? For a file, it can be {<operation>,
  <offset>, <size>, <data>/<buf>}. What about for a directory? FUSE
  expects the readdir implementation to populate directory entries one
  at a time, but the cache could hold data structures for directories
  in their entirety.
 */

class clientCache{
public:
  clientCache(uint64_t maxBytes = 1024*1024*64);

private:
  uint64_t maxBytes;
  //std::unordered_map<std::string, int> map;
  std::queue<writeRequest> writeQueue;
};

#endif
