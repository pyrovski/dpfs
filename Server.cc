#include <string>
#include <algorithm>

#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "Server.h"
#include "event.h"
#include "netListener.h"
#include "mon.pb.h"
#include "time.h"
#include "ServerContext.h"
#include "ServerConnection.h"
#include "util.h"

using namespace std;

Server::Server(uint16_t port, const char * logFile):
  port(port), log(logFile), base(NULL), listener(NULL)
{
  int status = loadOrCreateFSID(fsid);
  if(status)
    failmsg(log, "failed to load or create FSID");

  uuid_generate(uuid);
}

Server::~Server(){
  /* There should not be a case where we need multiple Servers
     launched in a single process. If there is, free some stuff.
   */
  quit();
}

struct event_base * Server::getBase(){
  return base;
}

void Server::setBase(struct event_base * base){
  this->base = base;
}

//!@todo fix
void Server::quit(){
  //delete context;
  //delete listener;
  // flush and close connections
  //!@todo segfault
  for(const auto& elem:connections)
    elem->close();
  event_base_loopbreak(base);
}

inline const log_t& Server::getLog() const{
  return const_cast<log_t&> (log);
}

void Server::registerConnection(ServerConnection *conn){
  dbgmsg(log, "registering connection: %p", conn);
  int status = conn->validate();
  if(status)
    connections.insert(conn);
  else {
    errmsg(log, "connection validation failure: %p", conn);
    log.flush();
    exit(1);
  }
}

const uint16_t Server::getPort() const {
  return port;
}

const uuid_t& Server::getFSID() const {
  return fsid;
}

const uuid_t& Server::getUUID() const {
  return uuid;
}

