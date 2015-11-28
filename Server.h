#ifndef SERVER_H
#define SERVER_H

#include <unordered_set>

#include <stdint.h>
#include <sys/socket.h>
#include <uuid/uuid.h>

#include "log.h"
#include "netListener.h"

class ServerConnection;

typedef void (*acceptCB_t)(evutil_socket_t , short , void * );

class Server {
 public:
  Server(uint16_t port, const char * logFile = 0);
  ~Server();
  virtual int run(bool foreground, acceptCB_t, void * acceptArg);
  virtual const uint16_t getPort() const;
  virtual void registerConnection(ServerConnection * conn);
  virtual void unregisterConnection(ServerConnection * conn);
  virtual const log_t& getLog() const;
  virtual const uuid_t& getFSID() const;
  virtual const uuid_t& getUUID() const;
  virtual struct event_base * getBase();
  virtual void setBase(struct event_base * base);
  
  virtual void quit();

 protected:
  struct event_base * base;
  uint16_t port;
  log_t log;
  netListener *listener;
  std::unordered_set<ServerConnection *> connections;
  uuid_t fsid;
  uuid_t uuid;
};

#endif
