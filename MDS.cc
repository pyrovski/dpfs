#include "MDS.h"
#include "util.h"
#include "ServerContext.h"

static void errorCB(struct bufferevent * bev, short error, void * arg){
}

static void readCB(struct bufferevent * bev, void * arg){
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
}

int MDS::run(bool foreground){
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
  
  return 0;
}
