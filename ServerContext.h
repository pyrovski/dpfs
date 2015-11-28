#ifndef SERVERCONTEXT_H
#define SERVERCONTEXT_H

class Server;

//!@todo this is just a pointer; remove it.
class ServerContext {
 public:
 ServerContext(): parent(0) {
  }
  
 ServerContext(Server * parent): parent(parent) {
  }
  
  virtual Server * getParent();
  virtual void setParent(Server *);
  
 protected:  
  Server * parent;
};

#endif
