#ifndef MONITOR_H
#define MONITOR_H

#include "Server.h"

class Monitor : public Server {
 public:
  using Server::Server;
  int run(bool foreground);
};

#endif
