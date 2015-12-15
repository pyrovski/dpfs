#include <string>
#include <algorithm>

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "event.h"
#include "netListener.h"
#include "mon.pb.h"
#include "time.h"
#include "MonitorConnection.h"
#include "Monitor.h"
#include "util.h"
#include "defaults.h"

using namespace std;

Monitor::Monitor(uint16_t port, const char * confFile):
  Server::Server(port, confFile)
{
  int status;
  char fsidStr[37];
  uuid_unparse(fsid, fsidStr);
  string path = buildConfPath(NULL, fsidStr) + "/" + defaultFSInitFile;
  int fd = open(path.c_str(), O_RDONLY);
  if(fd < 0)
    failmsg("Failed to open FS init file %s: %s", path.c_str(),
	    strerror(errno));

  evbuffer * buf = evbuffer_new();

  do{
    status = evbuffer_read(buf, fd, defaultReadSize);
  } while(status > 0);

  if(status < 0)
    failmsg("Failed to read FS init file %s: %s", path.c_str(),
	    strerror(errno));
  
  status = evbuffer_to_message(buf, fsOptions);

  // cleanup:
  if(fd >= 0)
    close(fd);
  evbuffer_free(buf); 
}

static void errorCB(struct bufferevent *bev, short error, void *arg){
  //!@todo check errors
  ServerConnection * conn = (ServerConnection*) arg;
  Server * parent = conn->getParent();
  dbgmsg("bufferevent error: 0x%x", error);

  bufferevent_free(bev);
  parent->unregisterConnection(conn);
}

static void acceptCB(evutil_socket_t socket, short flags, void * arg){
  Monitor * parent = (Monitor *) arg;
  dbgmsg("flags: 0x%x", flags);
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);

  dbgmsg("accepting");
  
  int fd = accept(socket, (struct sockaddr*)&ss, &slen);
  dbgmsg("accepted: %d", fd);
  if (fd < 0) {
    errmsg("accept: %d: %s", errno, strerror(errno));
  } else if (fd > FD_SETSIZE) {
    errmsg("fd outside set size");
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
