#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include "MonClient.h"

using namespace std;

MonClient::MonClient(const char * logFile): log(logFile) {
}

int MonClient::connectToServer(const char * address, uint16_t port){
  int status;
  string portStr = to_string(port);

  struct addrinfo *addressInfo;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
    
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd == -1){
    log.error("failed to open socket: %d", errno);
    goto fail;
  }
  
  evutil_make_socket_nonblocking(fd);
    
  status = 0;
  do {
    status = getaddrinfo(address, portStr.c_str(), &hints, &addressInfo);
  } while (status != 0 || status == EAI_AGAIN);

  if(status != 0){
    log.error("failed to get address info: %d", status);
    goto fail;
  }
  
  for(; addressInfo; addressInfo = addressInfo->ai_next){
    
    status = connect(fd, addressInfo->ai_addr, sizeof(*addressInfo->ai_addr));
    if(status == -1){
      log.error("failed to connect to %s:%d: %d", address, port, errno);
      continue;
    } else
      break;
  }

  if(status != 0)
    goto fail;
  
  // connected
  
  freeaddrinfo(addressInfo);

  clientSocket = fd;
  return 0;

 fail:
  if(fd >= 0)
    close(fd);
  return -1;
}
