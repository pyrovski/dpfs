#include <arpa/inet.h>
#include <errno.h>
#include "netListener.h"

netListener::netListener(uint16_t port){
  struct sockaddr_in sin;
  int status;

  //!@todo also IPV6
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(port);

  listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(listenSocket == -1){
    fail = errno;
    return;
  } else
    fail = 0;

  evutil_make_socket_nonblocking(listenSocket);

  status = bind(listenSocket, (struct sockaddr*)&sin, sizeof(sin));
  if(status == -1){
    fail = errno;
    return;    
  }

  status = listen(listenSocket, 16);
  if(status == -1){
    fail = errno;
    return;
  }
}
