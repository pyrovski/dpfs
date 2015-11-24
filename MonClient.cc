#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "MonClient.h"
#include "mon.pb.h"

using namespace std;
using namespace google::protobuf::io;

MonClient::MonClient(const char * logFile, int timeoutSeconds):
  log(logFile), timeoutSeconds(timeoutSeconds){
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

int MonClient::request(const char * path, struct stat * result){
  if(clientSocket < 0){
    log.error("not connected!");
    return -1;
  }
  
  mon::Query query;
  pbTime::Time tv_pb;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  tv_pb.set_seconds(tv.tv_sec);
  tv_pb.set_microseconds(tv.tv_usec);
  
  *(query.mutable_time()) = tv_pb;

  //!@todo send query
  int size = query.ByteSize();
  char *pkt = new char [size];
  google::protobuf::io::ArrayOutputStream aos(pkt,size);
  CodedOutputStream coded_output(&aos);
  query.SerializeToCodedStream(&coded_output);
  int status = send(clientSocket, pkt, size, 0);
  delete pkt;
  if(status == -1){
    log.error("send failure: %d", errno);
    return -1;
  }
  if(status != size){
    log.error("send failure: %d/%d", status, size);
    return -1;
  }
  return 0;
}
