#include "MDS.h"
#include "util.h"
#include "ServerContext.h"

static void errorCB(struct bufferevent * bev, short error, void * arg){
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  MDS * parent = (MDS *) arg;
  //!@todo
}

int MDS::run(bool foreground){

  return Server::run(foreground, &acceptCB, this);
}
