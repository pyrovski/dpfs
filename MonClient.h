#ifndef MONCLIENT_H
#define MONCLIENT_H

#include "event.h"
#include "log.h"

class MonClient {
 public:
  MonClient(const char * logFile = 0);
  int connectToServer(const char * address, uint16_t port);
  
 private:
  evutil_socket_t clientSocket;
  log_t log;
};

#endif
