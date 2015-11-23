#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>

#include "log.h"
#include "netListener.h"
#include "mon.pb.h"

class monitor {
 public:
  monitor(uint16_t port, const char * logFile = 0);
  ~monitor();
  void run(bool foreground = false);
  void printf(const char *, ...);

 private:
  uint32_t port;
  log_t log;
  netListener *listener;
  //  void *context;
};

#endif
