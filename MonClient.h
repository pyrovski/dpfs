#ifndef MONCLIENT_H
#define MONCLIENT_H

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <uuid/uuid.h>
#include <stdint.h>

#include "event.h"
#include "log.h"
#include "defaults.h"
#include "Reader.h"
#include "time.h"

class MonManager;

typedef enum {
  MonClientStateDefault = 0,
  MonClientStateSentRequest,
  MonClientStateReceivedSize
} MonClientState;

class MonClient: public Reader {
 public:
  MonClient(const log_t & log,
	    MonManager & parent,
	    const char * address, uint16_t port,
	    int timeoutSeconds = defaultClientTimeoutSeconds);

  ~MonClient();
  
  int connect();
  int connectNext();

  int request();

  int getFSID(uuid_t &fsid);

  void quit();
  bool isRunning();
  inline void setConnected() {connected = true;}
  inline void setDisconnected() {connected = false;}
  const log_t & getLog() const;
  bool enoughBytes() const;
  void processInput();
  int getState() const;
  
 private:
  struct addrinfo * addressInfo, * addressInfoBase;
  std::string address;
  uint16_t port;

  void setFSID(const uuid_t &fsid);
  const log_t & log;
  struct bufferevent * bev;
  uuid_t fsid;
  bool fsid_set;
  bool connected;
  MonManager & parent;
  int timeoutSeconds;
  int state;
  uint32_t incomingSize;
  pbTime::Time tv_query;
};

#endif
