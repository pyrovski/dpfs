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

void MonitorConnection::processInput(){
  struct evbuffer * input = bufferevent_get_input(bev);
  const log_t &log = parent->getLog();
  
  if(state == monitorConnStateDefault){
    uint32_t nSize;

    int status = evbuffer_remove(input, &nSize, sizeof(nSize));
    if(status == -1){
      dbgmsg(log, "read error: %d", status);
      return;
    } else
      dbgmsg(log, "read %d bytes on socket %d",
	     status, socket);

    if(status != sizeof(nSize)){
      errmsg(log, "read %d/%d on socket %d",
	     status, sizeof(nSize), socket);
      return;
    }    
    incomingSize = ntohl(nSize);

    if(incomingSize == 0)
      errmsg(log, "expected nonzero size");
    
    state = monitorConnStateReceivedSize;
  } else if(state == monitorConnStateReceivedSize){
    struct evbuffer * output = bufferevent_get_output(bev);

    //!@todo split
    
    //!@todo read request, send response
    mon::Query query;

    uint8_t * pkt = new uint8_t[incomingSize];
    int status =
      evbuffer_remove(input, pkt, incomingSize);
    if(status == -1){
      dbgmsg(log, "read error: %d: %s", errno, strerror(errno));
      return;
    } else
      dbgmsg(log, "read %d bytes on socket %d", status, socket);

    state = monitorConnStateDefault;
    
    //!@todo build query, get time from query, drain input
    delete pkt;
    
    mon::Response response;
    pbTime::Time tv_pb;
    getTime(tv_pb);
    *response.mutable_time() = tv_pb;
    
    string uuidStr((const char *)parent->getUUID(), sizeof(uuid_t));
    string fsidStr((const char *)parent->getFSID(), sizeof(uuid_t));
    *response.mutable_uuid() = uuidStr;
    *response.mutable_fsid() = fsidStr;

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
      errmsg(log, "getaddrinfo failed: %d", status);
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
      int length = max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
      char addrStr[length];
      void * sockaddr_addr =
	addressInfo->ai_family == AF_INET ?
	(void *)&((struct sockaddr_in*)addressInfo->ai_addr)->sin_addr.s_addr :
	(void *)&((struct sockaddr_in6*)addressInfo->ai_addr)->sin6_addr.s6_addr ;
      const char * result =
	inet_ntop(addressInfo->ai_family,
		  sockaddr_addr,
		  addrStr, length);
      dbgmsg(log, "mon %s:%d", addrStr, parent->getPort());
#endif
      if(addressInfo->ai_family == AF_INET){
	const auto &address = ((struct sockaddr_in*)addressInfo->ai_addr)->sin_addr.s_addr;
	monAddress.set_sa_addr((void*)&address, sizeof(address));
#ifdef DEBUG
	const char * tmp = monAddress.sa_addr().data();
#endif
      } else if(addressInfo->ai_family == AF_INET6){
	// addressInfo->ai_addr is in network byte order
	const auto &address = ((struct sockaddr_in6*)addressInfo->ai_addr)->sin6_addr.s6_addr;
	monAddress.set_sa_addr((void*)&address, sizeof(address));
	assert(sizeof(address) == 16);
	dbgmsg(log, "IPV6 address size: %d bytes", sizeof(address));
      } else {
	errmsg(log, "unknown address type: %d", addressInfo->ai_family);
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
      errmsg(log, "Failed to add message to output buffer");

    delete pkt;
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
    errmsg(parent->getLog(), "unknown state: %d", state);
    return false;
  }
}

const log_t & MonitorConnection::getLog() const {
  return parent->getLog();
}
