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

void monitor::printf(const char * str, ...){
  va_list vl;
  va_start(vl, str);
  log.printf(str, vl);
  va_end(vl);  
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  monitor * parent = (monitor *)arg;
  parent->printf("flags: 0x%x", flags);
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

  base = event_base_new();
  if(!base)
    log.fail("failed to open event base.");
  
  struct event * listenerEvent =
    event_new(base, listener->getSocketID(),
	      EV_READ | EV_PERSIST,
              &acceptCB, (void *) this);
  if(!listenerEvent)
    log.fail("failed to create listener event. Socket: %d, base: %p",
	     listener->getSocketID(), base);

  if(event_add(listenerEvent, NULL))
    log.fail("failed to add listener event");

  log.print("starting");
  log.flush();
  status = event_base_dispatch(base);
  log.printf("exiting: %d", status);
}
