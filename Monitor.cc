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
  ServerContext * context = (ServerContext*) arg;
  Server * parent = context->getParent();
  const log_t &log = parent->getLog();
  dbgmsg(log, "bufferevent error: 0x%x", error);

  bufferevent_free(bev);
}

static void readCB(struct bufferevent *bev, void *arg){
  MonitorConnection * connection = (MonitorConnection*) arg;
  Server * parent = connection->getParent();
  const log_t &log = parent->getLog();

  struct evbuffer * input = bufferevent_get_input(bev);

  dbgmsg(log, "%s: conn: %p, state: %d",
	 __FUNCTION__, connection, connection->getState());

  while(connection->enoughBytes(input))
    connection->processInput(input);
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  ServerContext * context = (ServerContext*) arg;
  Server * parent = context->getParent();
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
    MonitorConnection * monConnection = new MonitorConnection(*context);
    monConnection->setSocket(fd);
    monConnection->setSS(ss);
    monConnection->setBEV(bev);
    bufferevent_setcb(bev, readCB, NULL, errorCB, (void *)monConnection);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    parent->registerConnection(monConnection);
  }
}

int Monitor::run(bool foreground){

  int status;

  if(!foreground){
    daemonize(log);
  } else // foreground
    dbgmsg(log, "foreground");

  listener = new netListener(port);
  if(!listener)
    failmsg(log, "failed to allocate listen socket");

  if(listener->failed()){
    errmsg(log, "failed to create listen socket: %d", listener->failed());
    return -1;
  }

  ServerContext *context = new ServerContext;
  context->setParent(this);
  setBase(event_base_new());
  if(!getBase())
    failmsg(log, "failed to open event base.");
  
  struct event * listenerEvent =
    event_new(getBase(), listener->getSocketID(),
	      EV_READ | EV_PERSIST,
              &acceptCB, (void *) context);
  if(!listenerEvent)
    failmsg(log, "failed to create listener event. Socket: %d, base: %p",
	    listener->getSocketID(), getBase());

  if(event_add(listenerEvent, NULL))
    failmsg(log, "failed to add listener event");

  logmsg(log, "starting");
  log.flush();
  status = event_base_dispatch(getBase());
  logmsg(log, "exiting: %d", status);
  log.flush();
}
