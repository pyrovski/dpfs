#include "ServerContext.h"
#include "Server.h"

Server * ServerContext::getParent(){
  return parent;
}

void ServerContext::setParent(Server * parent){
  this->parent = parent;
}
