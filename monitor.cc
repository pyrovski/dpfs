#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "monitor.h"
#include "event.h"
#include "netListener.h"

monitor::monitor(uint16_t port, const char * logFile):
  port(port), log(logFile)
{
}

monitor::~monitor(){
  /* There should not be a case where we need multiple monitors
     launched in a single process. If there is, free some stuff.
   */
  //delete context;
  //delete listener;
}

void monitor::printf(const char * str, ...){
  va_list vl;
  va_start(vl, str);
  log.printf(str, vl);
  va_end(vl);
  log.flush();
}

typedef struct {
  struct event_base *base;
  monitor * mon;
} monitorContext;

static void errorcb(struct bufferevent *bev, short error, void *arg){
  //!@todo check errors
  monitorContext * context = (monitorContext*) arg;
  monitor * parent = context->mon;
  struct event_base *base = context->base;

  bufferevent_free(bev);
}

static void readcb(struct bufferevent *bev, void *arg){
  monitorContext * context = (monitorContext*) arg;
  monitor * parent = context->mon;
  struct event_base *base = context->base;

  struct evbuffer * input, * output;

  //!@todo read request, send response
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  monitorContext * context = (monitorContext*) arg;
  monitor * parent = context->mon;
  parent->printf("flags: 0x%x", flags);
  struct event_base *base = context->base;
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(socket, (struct sockaddr*)&ss, &slen);
  if (fd < 0) {
    perror("accept");
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    struct bufferevent *bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
  }
}

void monitor::run(bool foreground){

  int status;

  if(!foreground){
    pid_t pid = fork();
    if(!pid){
      // child
      pid = setsid();
      if(pid == -1)
	log.fail("failed setsid.");

      log.dbg("monitor forked");
      {
        const int fd = open ("/dev/null", O_RDWR, 0);
        dup2 (fd, STDIN_FILENO);
        dup2 (fd, STDOUT_FILENO);
        dup2 (fd, STDERR_FILENO);
        close (fd);
      }
    } else if(pid > 0){
      // parent
      exit(0);
    } else if(pid < 0)
      log.fail("failed to fork.");
  } else // foreground
    log.dbg("monitor foreground");

  listener = new netListener(port);
  if(!listener)
    log.fail("failed to allocate listen socket");

  if(listener->failed())
    log.fail("failed to create listen socket: %d", listener->failed());

  monitorContext *context = new monitorContext;
  context->mon = this;
  context->base = event_base_new();
  if(!context->base)
    log.fail("failed to open event base.");
  
  struct event * listenerEvent =
    event_new(context->base, listener->getSocketID(),
	      EV_READ | EV_PERSIST,
              &acceptCB, (void *) context);
  if(!listenerEvent)
    log.fail("failed to create listener event. Socket: %d, base: %p",
	     listener->getSocketID(), context->base);

  if(event_add(listenerEvent, NULL))
    log.fail("failed to add listener event");

  log.print("starting");
  log.flush();
  status = event_base_dispatch(context->base);
  log.printf("exiting: %d", status);
  log.flush();
}
