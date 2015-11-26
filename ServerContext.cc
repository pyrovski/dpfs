#include "ServerContext.h"

Server * ServerContext::getParent(){
  return parent;
}

void ServerContext::setParent(Server * parent){
  this->parent = parent;
}

struct event_base * ServerContext::getBase(){
  return base;
}

void ServerContext::setBase(struct event_base * base){
  this->base = base;
}
