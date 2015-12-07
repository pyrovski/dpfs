#include <string.h>
#include "ServerConnection.h"
#include "log.h"
#include "Server.h"

void ServerConnection::init(){
  memset(&ss, 0, sizeof(ss));
  socket = -1;
  bev = NULL;
  state = 0;
  incomingSize = 0;
}

void ServerConnection::close(){
  bufferevent_free(bev);
  bev = NULL;
}

int ServerConnection::getState() const {
  return state;
}

int ServerConnection::setSocket(int socket){
  this->socket = socket;
}

void ServerConnection::setSS(struct sockaddr_storage &ss){
  this->ss = ss;
}

void ServerConnection::setBEV(struct bufferevent *bev){
  this->bev = bev;
}

void genericReadCB(struct bufferevent *bev, void *arg){
  Reader * connection = (Reader *) arg;
  const log_t &log = connection->getLog();
  dbgmsg(log, "%s: conn: %p, state: %d",
	 __FUNCTION__, connection, connection->getState());
  
  while(connection->enoughBytes())
    connection->processInput();
}

const log_t & ServerConnection::getLog() const {
  return parent->getLog();
}
