#ifndef MONITOR_CONNECTION_H
#define MONITOR_CONNECTION_H
#include "event.h"
#include "ServerConnection.h"

typedef enum {
  monitorConnStateMin = -1,
  monitorConnStateDefault = 0,
  monitorConnStateReceivedSize,
  monitorConnStateMax
} MonitorConnState;

class MonitorConnection : public ServerConnection {
 public:
 MonitorConnection()
    {
      init();
    }
 MonitorConnection(const ServerContext &context):
  ServerConnection(context)
  {
    init();
  }
  bool enoughBytes(const struct evbuffer *) const;
  int validate() const;

  void processInput(struct evbuffer * input);

 private:
  void init();
};

inline void MonitorConnection::init(){
  ServerConnection::init();
  state = monitorConnStateDefault;
}

#endif
