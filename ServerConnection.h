#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H
#include "event.h"
#include "ServerContext.h"
#include "Reader.h"

/*!@param arg pointer to ServerConnection
 */
void genericReadCB(struct bufferevent *bev, void *arg);

class ServerConnection : public ServerContext, public Reader {
 public:
 ServerConnection()
    {
      init();
    }
 ServerConnection(const ServerContext &context):
  ServerContext(context)
  {
    init();
  }

  virtual bool enoughBytes() const = 0;
  virtual int validate() const = 0;

  virtual void processInput() = 0;
  virtual void close();
  virtual int getState() const;
  virtual void setSocket(int);
  virtual void setSS(struct sockaddr_storage &);
  virtual void setBEV(struct bufferevent *bev);
  
 protected:
  evutil_socket_t socket;
  struct sockaddr_storage ss;
  struct bufferevent * bev;
  int state;
  uint32_t incomingSize;
  virtual void init();
};

#endif
