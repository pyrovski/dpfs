#ifndef MONITOR_H
#define MONITOR_H

#include "Server.h"

class Monitor : public Server {
 public:
  /*!@todo in monitor constructor, read init file from FS. If none
    exists, fail. */
  Monitor(uint16_t port, const char * confFile = 0);
  int run(bool foreground);
};

#endif
