#include "OSD.h"
#include "util.h"
#include "ServerContext.h"
#include "MonClient.h"

static void errorCB(struct bufferevent * bev, short error, void * arg){
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  OSD * parent = (OSD *) arg;
  //!@todo
}

OSD::~OSD(){
  delete db;
}

int OSD::run(bool foreground){
  /*!@todo connect to monitors specified in config key "monitors"
   */

  
  
  return Server::run(foreground, &acceptCB, this);
}
