#include "clientCache.h"

using namespace std;

clientCache::clientCache(uint64_t maxBytes){
  this->maxBytes = maxBytes;
  this->currentBytes = 0;
}
