#ifndef MONITOR_CONNECTION_H
#define MONITOR_CONNECTION_H
#include "event.h"
#include "ServerConnection.h"

/*!@todo implement method to notify clients that relevant data has
   changed; e.g. PG map.
 */

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
  bool enoughBytes() const; //!@todo this should be generic
  int validate() const;

  void processInput();
  const log_t & getLog() const;

 private:
  void init();
};

inline void MonitorConnection::init(){
  ServerConnection::init();
  state = monitorConnStateDefault;
}
#endif
