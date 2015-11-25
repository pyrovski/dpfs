#include <string>
#include <algorithm>

#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "monitor.h"
#include "event.h"
#include "netListener.h"
#include "mon.pb.h"
#include "time.h"
#include "monitorContext.h"
#include "monitorConnection.h"
#include "util.h"

using namespace std;

monitor::monitor(uint16_t port, const char * logFile):
  port(port), log(logFile)
{
  int status = loadOrCreateFSID(fsid);
  if(status)
    failmsg(log, "failed to load or create FSID");

  uuid_generate(uuid);
}

monitor::~monitor(){
  /* There should not be a case where we need multiple monitors
     launched in a single process. If there is, free some stuff.
   */
  //delete context;
  //delete listener;
  // flush and close connections
}

inline const log_t& monitor::getLog() const{
  return const_cast<log_t&> (log);
}

void monitor::registerConnection(const monitorConnection *conn){
  dbgmsg(log, "registering connection: %p", conn);
  int status = conn->validate();
  if(status)
    connections.insert(conn);
  else {
    errmsg(log, "connection validation failure: %p", conn);
    log.flush();
    exit(1);
  }
}

static void errorcb(struct bufferevent *bev, short error, void *arg){
  //!@todo check errors
  monitorContext * context = (monitorContext*) arg;
  monitor * parent = context->mon;
  struct event_base *base = context->base;

  bufferevent_free(bev);
}

static void readcb(struct bufferevent *bev, void *arg){
  monitorConnection * connection = (monitorConnection*) arg;
  monitor * parent = connection->mon;
  const log_t &log = parent->getLog();
  struct event_base *base = connection->base;

  struct evbuffer * input = bufferevent_get_input(bev);

  dbgmsg(log, "%s: conn: %p, state: %d",
	 __FUNCTION__, connection, connection->state);

  while(connection->enoughBytes(input))
    connection->processInput(input);
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  monitorContext * context = (monitorContext*) arg;
  monitor * parent = context->mon;
  const log_t &log = parent->getLog();
  dbgmsg(log, "flags: 0x%x", flags);
  struct event_base *base = context->base;
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
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    monitorConnection * monConnection = new monitorConnection(*context);
    monConnection->socket = fd;
    monConnection->ss = ss;
    monConnection->bev = bev;
    bufferevent_setcb(bev, readcb, NULL, errorcb, (void *)monConnection);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    parent->registerConnection(monConnection);
  }
}

int monitor::run(bool foreground){

  int status;

  if(!foreground){
    daemonize(log);
  } else // foreground
    dbgmsg(log, "monitor foreground");

  listener = new netListener(port);
  if(!listener)
    failmsg(log, "failed to allocate listen socket");

  if(listener->failed()){
    errmsg(log, "failed to create listen socket: %d", listener->failed());
    return -1;
  }

  monitorContext *context = new monitorContext;
  context->mon = this;
  context->base = event_base_new();
  if(!context->base)
    failmsg(log, "failed to open event base.");
  
  struct event * listenerEvent =
    event_new(context->base, listener->getSocketID(),
	      EV_READ | EV_PERSIST,
              &acceptCB, (void *) context);
  if(!listenerEvent)
    failmsg(log, "failed to create listener event. Socket: %d, base: %p",
	     listener->getSocketID(), context->base);

  if(event_add(listenerEvent, NULL))
    failmsg(log, "failed to add listener event");

  logmsg(log, "starting");
  log.flush();
  status = event_base_dispatch(context->base);
  logmsg(log, "exiting: %d", status);
  log.flush();
}
