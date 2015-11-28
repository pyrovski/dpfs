#include "MDS.h"
#include "util.h"
#include "ServerContext.h"

static void errorCB(struct bufferevent * bev, short error, void * arg){
}

static void readCB(struct bufferevent * bev, void * arg){
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  MDS * parent = (MDS *) arg;
}

int MDS::run(bool foreground){

  return Server::run(foreground, &acceptCB, this);
}
