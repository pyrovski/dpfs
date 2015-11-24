#ifndef MONITOR_H
#define MONITOR_H

#include <unordered_set>

#include <stdint.h>
#include <sys/socket.h>

#include "log.h"
#include "netListener.h"
#include "mon.pb.h"
#include "monitorConnection.h"

class monitor {
 public:
  monitor(uint16_t port, const char * logFile = 0);
  ~monitor();
  int run(bool foreground = false);
  uint32_t getPort() const;
  void registerConnection(const monitorConnection *conn);
  const log_t& getLog() const;

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
