#ifndef MONCLIENT_H
#define MONCLIENT_H

#include <condition_variable>
#include <mutex>
#include <thread>

#include <uuid/uuid.h>

#include "event.h"
#include "log.h"
#include "defaults.h"
#include "Reader.h"

class MonManager;

typedef enum {
  MonClient_default = 0;
} MonClientState;

class MonClient: public Reader {
 public:
  MonClient(const log_t & log,
	    int timeoutSeconds = defaultClientTimeoutSeconds,
	    MonManager * parent);
  int connectToServer(const char * address, uint16_t port);
  int connectNext();

  //!@todo finish
  int request();

  int getFSID(uuid_t &fsid);

  void quit();
  bool isRunning();
  const log_t & getLog() const;
  bool enoughBytes() const;
  void processInput();
  int getState() const;
  
 private:
  MonClient();
  struct addrinfo * addressInfo, * addressInfoBase;

  void setFSID(const uuid_t &fsid);
  const log_t & log;
  struct bufferevent * bev;
  uuid_t fsid;
  bool fsid_set;
  bool running;
  bool connected;
  MonManger & parent;
};

#endif
