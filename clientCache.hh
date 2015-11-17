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
  int addEntry();
  void dropEntry();
  bool isCached(const char * path, size_t size, off_t offset);

private:
  uint64_t maxBytes;
  uint64_t currentBytes;
  //std::unordered_map<std::string, int> map;
  std::queue<writeRequest> writeQueue;
};

#endif
