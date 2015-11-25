#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <google/protobuf/io/coded_stream.h>
//#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "MonClient.h"
#include "mon.pb.h"
#include "time.h"

using namespace std;
using namespace google::protobuf::io;

MonClient::MonClient(const char * logFile, int timeoutSeconds):
  log(logFile), timeoutSeconds(timeoutSeconds), clientSocket(-1) {
}

int MonClient::connectToServer(const char * address, uint16_t port){
  int status;
  string portStr = to_string(port);
  dbgmsg(log, "connecting to %s:%d", address, port);

  struct addrinfo *addressInfo;
  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  //evutil_make_socket_nonblocking(fd);
    
  int fd = -1;
  status = 0;
  do {
    status = getaddrinfo(address, portStr.c_str(), &hints, &addressInfo);
  } while (status != 0 || status == EAI_AGAIN);

  if(status != 0){
    errmsg(log, "failed to get address info: %d", status);
    goto fail;
  }

  for(; addressInfo; addressInfo = addressInfo->ai_next){
    
    fd = socket(addressInfo->ai_family, SOCK_STREAM, 0);
    if(fd == -1){
      errmsg(log, "failed to open socket: %d: %s", errno, strerror(errno));
      goto fail;
    }
    status = connect(fd, addressInfo->ai_addr, sizeof(*addressInfo->ai_addr));
    if(status == -1){ //  && errno != EINPROGRESS
      errmsg(log, "failed to connect to %s:%d: %d: %s", address, port, errno,
	     strerror(errno));
      close(fd);
      continue;
    } else
      break;
  }

  if(status != 0) //  && errno != EINPROGRESS
    goto fail;
  
  // connected or in progress
  
  freeaddrinfo(addressInfo);

  clientSocket = fd;
  return 0;

 fail:
  if(fd >= 0)
    close(fd);
  return -1;
}

int MonClient::request(){
  if(clientSocket < 0){
    errmsg(log, "not connected!");
    return -1;
  }
  
  mon::Query query;
  pbTime::Time tv_query;
  getTime(tv_query);
  *query.mutable_time() = tv_query;

  int size = query.ByteSize();
  uint8_t *pkt = new uint8_t[size];
  google::protobuf::io::ArrayOutputStream aos(pkt, size);
  CodedOutputStream coded_output(&aos);
  query.SerializeToCodedStream(&coded_output);

  int status;

  uint32_t nSize = htonl(size);
  //!@todo function to print sockaddr
  dbgmsg(log, "sending %d bytes (%d) to socket %d",
	 sizeof(nSize), size, clientSocket);
  status = send(clientSocket, &nSize, sizeof(nSize), 0);
  if(status == -1){
    errmsg(log, "send failure: %d", errno);
    return -1;
  } else if(status != sizeof(nSize)){
    errmsg(log, "send failure: %d/%d", status, size);
    return -1;
  }
  
  dbgmsg(log, "sending %d bytes to socket %d", size, clientSocket);
  status = send(clientSocket, pkt, size, 0);
  delete pkt;
  if(status == -1){
    errmsg(log, "send failure: %d", errno);
    return -1;
  } else if(status != size){
    errmsg(log, "send failure: %d/%d", status, size);
    return -1;
  }

  // receive response

  uint32_t responseSize = 0;
  dbgmsg(log, "receiving %d bytes from socket %d",
	 sizeof(responseSize), clientSocket);

  status = recv(clientSocket, &responseSize, sizeof(responseSize), MSG_WAITALL);
  if(status == -1) //!@todo handle failure
    failmsg(log, "recv failure: %d", errno);

  if(status != sizeof(responseSize)) //!@todo handle failure
    failmsg(log, "recv failure: %d/%d", status, sizeof(responseSize));

  responseSize = ntohl(responseSize);
  dbgmsg(log, "received %d bytes: %d", sizeof(responseSize), responseSize);

  pkt = new uint8_t[responseSize];
  
  status = recv(clientSocket, pkt, responseSize, MSG_WAITALL);
  if(status == -1) //!@todo handle failure
    failmsg(log, "recv failure: %d", errno);

  if(status != responseSize) //!@todo handle failure
    failmsg(log, "recv failure: %d/%d", status, responseSize);
  
  dbgmsg(log, "received %d bytes", responseSize);

  mon::Response response;

  ArrayInputStream ais(pkt, responseSize);
  CodedInputStream coded_input(&ais);
  CodedInputStream::Limit msgLimit = coded_input.PushLimit(responseSize);

  //deserialize
  response.ParseFromCodedStream(&coded_input);
  coded_input.PopLimit(msgLimit);
  
  //!@todo handle response
  struct timeval tv, tvReq;
  tvFromPB(response.time(), tv);
  tvFromPB(tv_query, tvReq);
  
  dbgmsg(log, "req time: %es", tvDiff(tv, tvReq));

  const mon::Response_Mons &mons = response.mons();
  const mon::Response_OSDs &osds = response.osds();
  const mon::Response_MDSs &mdss = response.mdss();

#ifdef DEBUG
  for(int i = 0; i < mons.address_size(); ++i){
    if(mons.address(i).has_address4()){
      auto address = mons.address(1).address4();
      char addrStr[INET_ADDRSTRLEN];
      uint32_t addr4 = address.address();
      const char * result = inet_ntop(AF_INET, &addr4, addrStr, INET_ADDRSTRLEN);
      dbgmsg(log, "mon %d: %s:%d", i, addrStr, mons.address(i).port());
    }
  }
#endif
  return 0;
}
