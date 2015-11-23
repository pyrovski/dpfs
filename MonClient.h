#ifndef MONCLIENT_H
#define MONCLIENT_H

#include "event.h"

class MonClient {
 public:
  MonClient();
  int connect(const char * address);
  
 private:
  evutil_socket_t socket;
};

#endif
