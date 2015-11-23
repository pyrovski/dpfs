#ifndef NETLISTENER_H
#define NETLISTENER_H

#include "event.h"

class netListener{
 public:
  netListener(uint16_t port);
  int failed() const;
  evutil_socket_t getSocketID() const;
  
 private:
  evutil_socket_t listenSocket;
  int fail;
};

inline int netListener::failed() const{
  return fail;
}

inline evutil_socket_t netListener::getSocketID() const {
  return listenSocket;
}

#endif
