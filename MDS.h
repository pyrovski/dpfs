#ifndef MDS_H
#define MDS_H

#include "Server.h"

class MDS : public Server {
 public:
  using Server::Server;
  int run(bool foreground);
};

#endif
