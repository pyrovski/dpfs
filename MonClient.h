#ifndef MONCLIENT_H
#define MONCLIENT_H

#include "event.h"
#include "log.h"
#include "defaults.h"

class MonClient {
 public:
  MonClient(const char * logFile = 0, int timeoutSeconds = defaultClientTimeoutSeconds);
  int connectToServer(const char * address, uint16_t port);
  int request(const char * path, struct stat * result);
  
 private:
  evutil_socket_t clientSocket;
  log_t log;
  int timeoutSeconds;
};

#endif
