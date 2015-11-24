#ifndef MONITOR_H
#define MONITOR_H

#include <unordered_set>

#include <stdint.h>
#include <sys/socket.h>

#include "log.h"
#include "netListener.h"
#include "mon.pb.h"

class monitor;

typedef struct monitorContext {
  struct event_base * base;
  monitor * mon;
} monitorContext;

typedef struct monitorConnection : monitorContext {
  monitorConnection(const struct monitorContext &context):
    monitorContext(context)
  {
  }
  evutil_socket_t socket;
  struct sockaddr_storage ss;
  struct bufferevent * bev;
} monitorConnection;

class monitor {
 public:
  monitor(uint16_t port, const char * logFile = 0);
  ~monitor();
  void run(bool foreground = false);
  void printf(const char *, ...);
  void dbg(const char *, ...);
  void err(const char *, ...);
  uint32_t getPort() const;
  void registerConnection(const monitorConnection *conn);

 private:
  uint32_t port;
  log_t log;
  netListener *listener;
  std::unordered_set<const monitorConnection *> connections;
};

inline uint32_t monitor::getPort() const {
  return port;
}

#endif
