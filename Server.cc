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

Server::Server(uint16_t port, const char * logFile, const char * confFile):
  port(port), log(logFile), conf(&log, confFile), base(NULL), listener(NULL)
{
  int status;
  status = conf.load();
  if(status)
    errmsg(log, "failed to load config file %s", confFile);

  if(conf.hasKey("fsid")){
    status = uuid_parse(conf.get("fsid")->c_str(), fsid);
    if(status)
      errmsg(log, "failed to parse %s as fsid", conf.get("fsid")->c_str());
  }

  status = loadOrCreateFSID(fsid);
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
  const log_t &result = const_cast<log_t&> (log);
  assert(&result);
  return result;
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

void Server::unregisterConnection(ServerConnection *conn){
  dbgmsg(log, "unregistering connection: %p", conn);
  connections.erase(conn);
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

int Server::run(bool foreground, acceptCB_t acceptCB, void * acceptArg){
  int status;

  if(!foreground){
    daemonize(log);
  } else // foreground
    dbgmsg(log, "foreground");

  listener = new netListener(port);
  if(!listener)
    failmsg(log, "failed to allocate listen socket");

  if(listener->failed()){
    errmsg(log, "failed to create listen socket: %d", listener->failed());
    return -1;
  }

  setBase(event_base_new());
  if(!getBase())
    failmsg(log, "failed to open event base.");
  
  struct event * listenerEvent =
    event_new(base, listener->getSocketID(),
	      EV_READ | EV_PERSIST,
              acceptCB, acceptArg);
  if(!listenerEvent)
    failmsg(log, "failed to create listener event. Socket: %d, base: %p",
	    listener->getSocketID(), base);

  if(event_add(listenerEvent, NULL))
    failmsg(log, "failed to add listener event");

  logmsg(log, "starting");
  log.flush();
  status = event_base_dispatch(base);
  logmsg(log, "exiting: %d", status);
  log.flush();
  
  return 0;
}
