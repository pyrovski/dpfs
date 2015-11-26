#ifndef SERVERCONTEXT_H
#define SERVERCONTEXT_H

#include "event.h"

class Server;

class ServerContext {
 public:
 ServerContext():
  base(NULL), parent(NULL)
    {
    }

  virtual Server * getParent();
  virtual void setParent(Server *);
  virtual struct event_base * getBase();
  virtual void setBase(struct event_base * base);
  
 protected:
  
  struct event_base * base;
  Server * parent;
};

#endif
