#ifndef MONCLIENT_H
#define MONCLIENT_H

#include <condition_variable>
#include <mutex>


#include <uuid/uuid.h>

#include "event.h"
#include "log.h"
#include "defaults.h"

class MonClient {
 public:
  MonClient(const char * logFile = 0, int timeoutSeconds = defaultClientTimeoutSeconds);
  int connectToServer(const char * address, uint16_t port);

  //!@todo finish
  int request();

  int getFSID(uuid_t &fsid);
  
 private:
  evutil_socket_t clientSocket;
  log_t log;
  int timeoutSeconds;

  int setFSID(const uuid_t &fsid);
  std::mutex theMutex; // protects fsid, fsid_set, cv
  std::condition_variable cv;
  uuid_t fsid;
  bool fsid_set;
  
};

#endif
