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
#include <uuid/uuid.h>

#include "MonClient.h"
#include "mon.pb.h"
#include "time.h"
#include "sockaddr_util.h"

using namespace std;
using namespace google::protobuf::io;

MonClient::MonClient():
  fsid_set(false),
  running(false),
  addressInfo(NULL),
  addressInfoBase(NULL),
  connected(false)
{
}

MonClient::MonClient(const log_t & log, int timeoutSeconds, MonManager & parent):
  log(log), timeoutSeconds(timeoutSeconds), MonClient(), parent(parent)
{
  bev = parent.registerClient(this);
  bufferevent_setcb(bev, genericReadCB, NULL, eventCB, this);
  bufferevent_enable(bev, EV_READ|EV_WRITE);
}

MonClient::~MonClient(){
  quit();
  freeaddrinfo(addressInfo);
}

bool MonClient::isRunning(){
  return running;
}

void MonClient::quit(){
  connected = running = false;
  bufferevent_free(clients[client]);
  parent.unregisterClient(this);
}

/*! If client has retrieved FSID from monitor, return FSID to
    caller. Otherwise, wait for FSID from monitor first.
 */
int MonClient::getFSID(uuid_t &fsid){
  if(fsid_set){
    uuid_copy(fsid, this->fsid);
    return 0;
  } else
    return -1;
}

void MonClient::setFSID(const uuid_t &fsid){
  uuid_copy(this->fsid, fsid);
  fsid_set = true;
}

//!@todo handle timeout. Reconnect?
static void eventCB(struct bufferevent * bev, short events, void * arg){
  MonClient * client = (MonClient *) arg;
  if(events & BEV_EVENT_CONNECTED){
    set_tcp_no_delay(bufferevent_getfd(bev));
    connected = true;
  } else if(events & BEV_EVENT_ERROR){
    errmsg(client->getLog(), "failed to connect bev: %p", bev);
    connected = false;
    client->connectNext();
  }
}

int MonClient::connectToServer(const char * address, uint16_t port){
  int status;
  string portStr = to_string(port);
  dbgmsg(log, "connecting to %s:%d", address, port);

  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
    
  //int fd = -1;
  status = 0;
  do {
    //!@todo convert to libevent implementation
    status = getaddrinfo(address, portStr.c_str(), &hints, &addressInfo);
  } while (status != 0 || status == EAI_AGAIN);

  if(status != 0){
    errmsg(log, "failed to get address info: %d", status);
    goto fail;
  }
  addressInfoBase = addressInfo;
  
  return connectNext();

 fail:
  return -1;
}

int MonClient::connectNext(){  
  for(; addressInfo; addressInfo = addressInfo->ai_next){
    int status = bufferevent_socket_connect(bev,
					    addressInfo->ai_addr,
					    sizeof(*addressInfo->ai_addr));
    if(status != 0) {
      //!@todo void addrToString(const struct sockaddr *, string &)
      if(addressInfo->ai_family == AF_INET ||
	 addressInfo->ai_family() == AF_INET6){
	const int length = max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
	char addrStr[length];
	const char * result = inet_ntop(addressInfo->ai_family,
					addressInfo->ai_addr,
					addrStr, length);
	uint16_t port = SOCK_ADDR_PORT(addressInfo->ai_addr);
	errmsg(log, "connect attempt failed: %s:%d",
	       addrStr, port);
      } else
	errmsg(log, "expected IPV4 or IPV6 address");
    } else
      break;
  }

  if(status != 0)
    return -1;

  return 0;
}

int MonClient::request(){
  if(!running || !connected){
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
  dbgmsg(log, "sending %d bytes (%d)",
	 sizeof(nSize), size);
  struct evbuffer * output = bufferevent_get_output(bev);
  status = evbuffer_add(output, &nSize, sizeof(nSize));
  if(status == -1){
    errmsg(log, "send failure");
    /*
    if(errno == EPIPE)
      disconnect();
    */
    return -1;
  }
  
  dbgmsg(log, "sending %d bytes", size);
  status = evbuffer_add(output, pkt, size);
  delete pkt;
  if(status == -1){
    errmsg(log, "send failure");
    return -1;
  }

  return 0;
}

void MonClient::processInput(){
  // receive response

  uint32_t responseSize = 0;
  dbgmsg(log, "receiving %d bytes from socket %d",
	 sizeof(responseSize), clientSocket);

  struct evbuffer * input = bufferevent_get_input(bev);;

  //!@todo
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

  //const mon::Response_Mon &mons = response.mon();
  //const mon::Response_OSD &osds = response.osd();
  //const mon::Response_MDS &mdss = response.mds();

  if(response.fsid().capacity() < sizeof(uuid_t)){
    errmsg(log, "uuid error in monitor response");
  } else
    setFSID(*(const uuid_t*)response.fsid().data());

#ifdef DEBUG
  for(int i = 0; i < response.mon_size(); ++i){
    const mon::Response::Mon &Mon = response.mon(i);
    for(int j = 0; j < Mon.address_size(); ++j){
      auto address = Mon.address(j);
      if(address.sa_family() == AF_INET || address.sa_family() == AF_INET6){
	const int length = max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
	char addrStr[length];
	const char * result = inet_ntop(address.sa_family(),
					address.sa_addr().data(),
					addrStr, length);
	dbgmsg(log, "mon %d addr %d: %s:%d", i, j, addrStr, address.port());
      } else
	errmsg(log, "expected address4 or address6");
    }
  }
#endif

}
