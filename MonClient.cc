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
#include "MonManager.h"
#include "mon.pb.h"
#include "sockaddr_util.h"
#include "ServerConnection.h"
#include "string.h"
#include "util.h"

using namespace std;
using namespace google::protobuf::io;

//!@todo handle timeout. Reconnect?
static void eventCB(struct bufferevent * bev, short events, void * arg){
  MonClient * client = (MonClient *) arg;
  if(events & BEV_EVENT_CONNECTED){
    dbgmsg("connected bev: %p", bev);
    set_tcp_no_delay(bufferevent_getfd(bev));
    client->setConnected();
  } else if(events & BEV_EVENT_ERROR){
    errmsg("failed to connect bev: %p", bev);
    client->setDisconnected();
    client->connectNext();
  }
}

MonClient::MonClient(MonManager & parent,
		     const char * address, uint16_t port,
		     int timeoutSeconds):
  fsidValid(false),
  addressInfo(NULL),
  addressInfoBase(NULL),
  connected(false),
  timeoutSeconds(timeoutSeconds),
  parent(parent),
  state(MonClientStateDefault),
  address(address), port(port)
{
  bev = parent.registerClient(this);
  //!@todo fix genericReadCB to work with Reader *
  bufferevent_setcb(bev, genericReaderCB, NULL, eventCB, this);
  bufferevent_enable(bev, EV_READ|EV_WRITE);
}

MonClient::~MonClient(){
  quit();
  freeaddrinfo(addressInfo);
}

void MonClient::quit(){
  dbgmsg("MonClient %p quitting", this);
  connected = false;
  bufferevent_free(bev);
  parent.unregisterClient(this);
}

int MonClient::connect(){
  int status;
  string portStr = to_string(port);
  dbgmsg("connecting to %s:%d", address.c_str(), port);

  struct addrinfo hints;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if(addressInfo)
    freeaddrinfo(addressInfo);
  
  status = 0;
  do {
    //!@todo convert to libevent implementation
    status = getaddrinfo(address.c_str(), portStr.c_str(), &hints, &addressInfo);
  } while (status != 0 || status == EAI_AGAIN);

  if(status != 0){
    errmsg("failed to get address info: %d", status);
    goto fail;
  }
  addressInfoBase = addressInfo;
  
  return connectNext();

 fail:
  return -1;
}

int MonClient::connectNext(){
  int status = -1;
  for(; addressInfo; addressInfo = addressInfo->ai_next){
    status = bufferevent_socket_connect(bev,
					    addressInfo->ai_addr,
					    sizeof(*addressInfo->ai_addr));
    if(status != 0) {
      //!@todo void addrToString(const struct sockaddr *, string &)
      if(addressInfo->ai_family == AF_INET ||
	 addressInfo->ai_family == AF_INET6){
	const int length = max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
	char addrStr[length];
	const char * result = inet_ntop(addressInfo->ai_family,
					addressInfo->ai_addr,
					addrStr, length);
	//!@todo check result
	uint16_t port = SOCK_ADDR_PORT(addressInfo->ai_addr);
	errmsg("connect attempt failed: %s:%d",
	       addrStr, port);
      } else
	errmsg("expected IPV4 or IPV6 address");
    } else
      break;
  }
  
  if(status != 0)
    return -1;

  return 0;
}

int MonClient::request(){
  if(!connected){
    errmsg("not connected!");
    return -1;
  }
  if(state != MonClientStateDefault){
    errmsg("in progress");
    return -1;
  }

  mon::Query query;
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
  dbgmsg("sending %d bytes (%d)",
	 sizeof(nSize), size);
  struct evbuffer * output = bufferevent_get_output(bev);
  status = evbuffer_add(output, &nSize, sizeof(nSize));
  if(status == -1){
    errmsg("send failure");
    /*
    if(errno == EPIPE)
      disconnect();
    */
    return -1;
  }
  
  dbgmsg("sending %d bytes", size);
  status = evbuffer_add(output, pkt, size);
  delete[] pkt;
  if(status == -1){
    errmsg("send failure");
    return -1;
  }

  state = MonClientStateSentRequest;
  return 0;
}

bool MonClient::enoughBytes() const {
  if(!connected){
    errmsg("not connected!");
    return false;
  }

  struct evbuffer * buf = bufferevent_get_input(bev);
  size_t bytes = evbuffer_get_length(buf);
  switch(state){
  case MonClientStateDefault: //!@todo unexpected
  case MonClientStateSentRequest:
    return bytes >= sizeof(incomingSize);
  case MonClientStateReceivedSize:
    return bytes >= incomingSize;
  default:
    errmsg("unknown state: %d", state);
    return false;
  }
}

void MonClient::processInput(){
  if(!connected){
    errmsg("not connected!");
    return;
  }

  int status;  
  struct evbuffer * input = bufferevent_get_input(bev);
  uint32_t responseSize = 0;

  switch(state){
  case MonClientStateDefault: //!@todo unexpected
  case MonClientStateSentRequest:
    {
      dbgmsg("receiving %d bytes", sizeof(responseSize));
      
      status = evbuffer_remove(input, &responseSize, sizeof(responseSize));
      if(status == -1) //!@todo handle failure
	failmsg("recv failure: %d", errno);
      
      responseSize = ntohl(responseSize);
      dbgmsg("received %d bytes: %d", sizeof(responseSize), responseSize);
      incomingSize = responseSize;
      state = MonClientStateReceivedSize;
      break;
    }
  case MonClientStateReceivedSize:
    {
      uint8_t * pkt = new uint8_t[incomingSize];

      //!@todo this could be implemented as an evbuffer defrag
      //!followed by release() or somesuch
      status = evbuffer_remove(input, pkt, incomingSize);
      if(status == -1) //!@todo handle failure
	failmsg("recv failure: %d", errno);
    
      dbgmsg("received %d bytes", incomingSize);

      mon::Response response;
    
      ArrayInputStream ais(pkt, incomingSize);
      CodedInputStream coded_input(&ais);
      CodedInputStream::Limit msgLimit = coded_input.PushLimit(incomingSize);
    
      //deserialize
      response.ParseFromCodedStream(&coded_input);
      coded_input.PopLimit(msgLimit);
    
      //!@todo handle response
      struct timeval tv, tvReq;
      tvFromPB(response.time(), tv);
      tvFromPB(tv_query, tvReq);
    
      dbgmsg("req time: %es", tvDiff(tv, tvReq));
    
      //const mon::Response_Mon &mons = response.mon();
      //const mon::Response_OSD &osds = response.osd();
      //const mon::Response_MDS &mdss = response.mds();

      uuid_t remoteFSID;
      status = uuid_parse(response.fsid().c_str(), remoteFSID);
      if(status){
	errmsg("uuid error in monitor response");
	quit();
      } else {
	if(uuid_compare(parent.getFSID(), remoteFSID)){
	  errmsg("FSIDs differ; local: %s, remote: %s",
		 to_string(parent.getFSID()).c_str(),
		 response.fsid().c_str());
	  quit();
	}
      }
      fsidValid = true;
      
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
	    //!@todo check result
	    dbgmsg("mon %d addr %d: %s:%d", i, j, addrStr, address.port());
	  } else
	    errmsg("expected address4 or address6");
	}
      }
#endif
      delete[] pkt;
      state = MonClientStateDefault;
      break;
    }
  default:
    errmsg("unknown state: %d", state);
  } // switch state
}

int MonClient::getState() const {
  return state;
}
