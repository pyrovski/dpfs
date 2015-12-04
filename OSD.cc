#include "OSD.h"
#include "util.h"
#include "ServerContext.h"

static void errorCB(struct bufferevent * bev, short error, void * arg){
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  OSD * parent = (OSD *) arg;
  //!@todo
}

OSD::~OSD(){
  if(db)
    delete db;
}

int OSD::run(bool foreground){
  
  return Server::run(foreground, &acceptCB, this);
}
