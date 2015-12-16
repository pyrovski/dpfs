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
  //!@todo all clients should have a preset FSID.
  MonClient(MonManager & parent,
	    const char * address, uint16_t port,
	    int timeoutSeconds = defaultClientTimeoutSeconds);

  ~MonClient();
  
  int connect();
  int connectNext();

  int request();

  void quit();
  bool isRunning();
  inline void setConnected() {connected = true;}
  inline void setDisconnected() {connected = false;}
  bool enoughBytes() const;
  void processInput();
  int getState() const;
  
 private:
  struct addrinfo * addressInfo, * addressInfoBase;
  std::string address;
  uint16_t port;

  struct bufferevent * bev;
  bool fsidValid;
  bool connected;
  MonManager & parent;
  int timeoutSeconds;
  int state;
  uint32_t incomingSize;
  pbTime::Time tv_query;
};

#endif
