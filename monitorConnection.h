#ifndef MONITOR_CONNECTION_H
#define MONITOR_CONNECTION_H
#include "event.h"
#include "monitor.h"
#include "monitorContext.h"

typedef enum {
  monitorConnStateDefault,
  monitorConnStateReceivedSize
} monitorConnState;

class monitor;

class monitorConnection : public monitorContext {
 public:
 monitorConnection():
  socket(-1), bev(NULL), state(monitorConnStateDefault), incomingSize(0)
    {
      memset(&ss, 0, sizeof(ss));
    }
 monitorConnection(const monitorContext &context):
  monitorContext(context)
  {
  }

  bool enoughBytes(const struct evbuffer *) const;

  void processInput(struct evbuffer * input);
  evutil_socket_t socket;
  struct sockaddr_storage ss;
  struct bufferevent * bev;
  monitorConnState state;
  uint32_t incomingSize;
};

inline bool monitorConnection::enoughBytes(const struct evbuffer * buf) const {
  size_t bytes = evbuffer_get_length(buf);
  switch(state){
  case monitorConnStateDefault:
    return bytes >= sizeof(incomingSize);
  case monitorConnStateReceivedSize:
    return bytes >= incomingSize;
  default:
    errmsg(mon->getLog(), "unknown state: %d", state);
    return false;
  }
}
#endif
