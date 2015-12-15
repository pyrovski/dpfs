#include <string>

#include <arpa/inet.h>

#include "log.h"
#include "MonitorConnection.h"
#include "platform.h"
#include "time.h"
#include "mon.pb.h"
#include "Server.h"
#include "util.h"

using namespace std;

int MonitorConnection::validate() const {
  return
    //socket != -1 &&
    bev &&
    state > monitorConnStateMin &&
    state < monitorConnStateMax;
}

/*!@todo Allow zero-valued size for server-initiated communications.
 */
void MonitorConnection::processInput(){
  struct evbuffer * input = bufferevent_get_input(bev);
  
  if(state == monitorConnStateDefault){
    uint32_t nSize;

    int status = evbuffer_remove(input, &nSize, sizeof(nSize));
    if(status == -1){
      dbgmsg("read error: %d", status);
      return;
    } else
      dbgmsg("read %d bytes on socket %d",
	     status, socket);

    if(status != sizeof(nSize)){
      errmsg("read %d/%d on socket %d",
	     status, sizeof(nSize), socket);
      return;
    }    
    incomingSize = ntohl(nSize);

    if(incomingSize == 0)
      errmsg("expected nonzero size");
    
    state = monitorConnStateReceivedSize;
  } else if(state == monitorConnStateReceivedSize){
    struct evbuffer * output = bufferevent_get_output(bev);

    //!@todo split
    
    //!@todo read request, send response
    mon::Query query;

    assert(incomingSize > 0);
    uint8_t * pkt = new uint8_t[incomingSize];
    int status =
      evbuffer_remove(input, pkt, incomingSize);
    if(status == -1){
      dbgmsg("read error: %d: %s", errno, strerror(errno));
      return;
    } else
      dbgmsg("read %d bytes on socket %d", status, socket);

    state = monitorConnStateDefault;
    
    //!@todo build query, get time from query, drain input
    delete[] pkt;
    
    pbTime::Time tv_pb;
    mon::Response response;
    dbgmsg("Response: %p", &response);
    getTime(tv_pb);
    *response.mutable_time() = tv_pb;

    //!@todo fix
    string uuidStr = to_string(parent->getUUID());
    *response.mutable_uuid() = uuidStr;
    *response.mutable_fsid() = to_string(parent->getFSID());

    mon::Response::Mon monEntry;
    *monEntry.mutable_uuid() = uuidStr;
    
    // get list of addresses on monitor host.

    //!@todo If client is connected from localhost, don't filter out
    //!loopback addresses from getaddrinfo().
    string portStr = to_string(parent->getPort());
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    struct addrinfo *addressInfo = NULL;
    status = getaddrinfo(NULL, portStr.c_str(), &hints, &addressInfo);
    if(status != 0){
      errmsg("getaddrinfo failed: %d", status);
      return;
    }

    //!@todo split
    for(; addressInfo; addressInfo = addressInfo->ai_next){
      netAddress::Address monAddress;
      monAddress.set_port(parent->getPort());
      //!@todo get ipv4/ipv6, set address in monAddress, add
      //!monAddress to mon, add mon to response.
      monAddress.set_sa_family(addressInfo->ai_family);
#ifdef DEBUG
      const int length = max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
      char addrStr[length];
      void * sockaddr_addr =
	addressInfo->ai_family == AF_INET ?
	(void *)&((struct sockaddr_in*)addressInfo->ai_addr)->sin_addr.s_addr :
	(void *)&((struct sockaddr_in6*)addressInfo->ai_addr)->sin6_addr.s6_addr ;
      const char * result =
	inet_ntop(addressInfo->ai_family,
		  sockaddr_addr,
		  addrStr, length);
      dbgmsg("mon %s:%d", addrStr, parent->getPort());
#endif
      if(addressInfo->ai_family == AF_INET){
	const auto &address = ((struct sockaddr_in*)addressInfo->ai_addr)->sin_addr.s_addr;
	monAddress.set_sa_addr((void*)&address, sizeof(address));
      } else if(addressInfo->ai_family == AF_INET6){
	// addressInfo->ai_addr is in network byte order
	const auto &address = ((struct sockaddr_in6*)addressInfo->ai_addr)->sin6_addr.s6_addr;
	monAddress.set_sa_addr((void*)&address, sizeof(address));
	assert(sizeof(address) == 16);
	dbgmsg("IPV6 address size: %d bytes", sizeof(address));
      } else {
	errmsg("unknown address type: %d", addressInfo->ai_family);
	continue;
      }
    
      *monEntry.add_address() = monAddress;
    }
    *response.add_mon() = monEntry;

    //!@todo
    /*
    mon::Response::MDS mdss;
    mon::Response::OSD osds;
    *response.mutable_osds() = osds;
    *response.mutable_mdss() = mdss;
    */

    status = message_to_evbuffer(response, output);
    if(status)
      errmsg("Failed to add message to output buffer");

  }
}

bool MonitorConnection::enoughBytes() const {
  struct evbuffer * buf = bufferevent_get_input(bev);
  size_t bytes = evbuffer_get_length(buf);
  switch(state){
  case monitorConnStateDefault:
    return bytes >= sizeof(incomingSize);
  case monitorConnStateReceivedSize:
    return bytes >= incomingSize;
  default:
    errmsg("unknown state: %d", state);
    return false;
  }
}
