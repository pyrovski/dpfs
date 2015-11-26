#ifndef SERVER_H
#define SERVER_H

#include <unordered_set>

#include <stdint.h>
#include <sys/socket.h>
#include <uuid/uuid.h>

#include "log.h"
#include "netListener.h"

class ServerConnection;

class Server {
 public:
  Server(uint16_t port, const char * logFile = 0);
  ~Server();
  virtual int run(bool foreground = false) = 0;
  virtual const uint16_t getPort() const;
  virtual void registerConnection(ServerConnection *conn);
  virtual const log_t& getLog() const;
  virtual const uuid_t& getFSID() const;
  virtual const uuid_t& getUUID() const;
  virtual void quit();

 protected:
  uint16_t port;
  log_t log;
  netListener *listener;
  std::unordered_set<ServerConnection *> connections;
  uuid_t fsid;
  uuid_t uuid;
};

#endif
