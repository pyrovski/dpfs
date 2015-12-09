#ifndef OSD_H
#define OSD_H

//#include <leveldb/db.h>

#include "Server.h"

class OSD : public Server {
 public:
  ~OSD();
  using Server::Server;
  int run(bool foreground);

 private:
  //leveldb::DB* db;
};

#endif
