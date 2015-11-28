#include <string>
#include <algorithm>

#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "event.h"
#include "netListener.h"
#include "mon.pb.h"
#include "time.h"
#include "MonitorConnection.h"
#include "Monitor.h"
#include "util.h"

using namespace std;

static void errorCB(struct bufferevent *bev, short error, void *arg){
  //!@todo check errors
  ServerConnection * conn = (ServerConnection*) arg;
  Server * parent = conn->getParent();
  const log_t &log = parent->getLog();
  dbgmsg(log, "bufferevent error: 0x%x", error);

  bufferevent_free(bev);
  parent->unregisterConnection(conn);
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  Monitor * parent = (Monitor *) arg;
  const log_t &log = parent->getLog();
  dbgmsg(log, "flags: 0x%x", flags);
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);

  dbgmsg(log, "accepting");
  
  int fd = accept(socket, (struct sockaddr*)&ss, &slen);
  dbgmsg(log, "accepted: %d", fd);
  if (fd < 0) {
    errmsg(log, "accept: %d: %s", errno, strerror(errno));
  } else if (fd > FD_SETSIZE) {
    errmsg(log, "fd outside set size");
    close(fd);
  } else {
    struct bufferevent *bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(parent->getBase(), fd, BEV_OPT_CLOSE_ON_FREE);
    ServerContext context(parent);
    MonitorConnection * monConnection = new MonitorConnection(context);
    monConnection->setSocket(fd);
    monConnection->setSS(ss);
    monConnection->setBEV(bev);
    bufferevent_setcb(bev, genericReadCB, NULL, errorCB, (void *)monConnection);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    parent->registerConnection(monConnection);
  }
}

int Monitor::run(bool foreground){
  return Server::run(foreground, &acceptCB, this);
}
