#include <string>

#include <arpa/inet.h>

#include <google/protobuf/io/coded_stream.h>
//#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "log.h"
#include "monitorConnection.h"
#include "platform.h"
#include "time.h"

using namespace std;
using namespace google::protobuf::io;

int monitorConnection::validate() const {
  return
    //socket != -1 &&
    bev &&
    state > monitorConnStateMin &&
    state < monitorConnStateMax;
}

void monitorConnection::processInput(struct evbuffer * input){
  const log_t &log = mon->getLog();
  
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
    if(status == -1)
      dbgmsg(log, "read error: %d: %s", errno, strerror(errno));
    else
      dbgmsg(log, "read %d bytes on socket %d", status, socket);
    
    //!@todo build query, get time from query, drain input
    delete pkt;
    
    mon::Response response;
    pbTime::Time tv_pb;
    getTime(tv_pb);
    *response.mutable_time() = tv_pb;
    
    mon::Response::Mon monEntry;
    
    // get list of addresses on monitor host.

    //!@todo If client is connected from localhost, don't filter out
    //!loopback addresses from getaddrinfo().
    string portStr = to_string(mon->getPort());
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
    
    for(; addressInfo; addressInfo = addressInfo->ai_next){
      netAddress::Address monAddress;
      monAddress.set_port(mon->getPort());
      //!@todo get ipv4/ipv6, set address in monAddress, add
      //!monAddress to mon, add mon to response.
      monAddress.set_sa_family(addressInfo->ai_family);
      if(addressInfo->ai_family == AF_INET){
	const auto &address = ((struct sockaddr_in*)addressInfo->ai_addr)->sin_addr.s_addr;
	monAddress.set_sa_addr((void*)&address, sizeof(address));

#ifdef DEBUG
	char addrStr[INET_ADDRSTRLEN];
	const char * result =
	  inet_ntop(AF_INET,
		    &((struct sockaddr_in*)addressInfo->ai_addr)->sin_addr.s_addr,
		    addrStr, INET_ADDRSTRLEN);
	dbgmsg(log, "mon %s:%d", addrStr, mon->getPort());
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
    string uuidStr((const char *)mon->getUUID(), sizeof(uuid_t));
    string fsidStr((const char *)mon->getFSID(), sizeof(uuid_t));
    *response.mutable_uuid() = uuidStr;
    *response.mutable_fsid() = fsidStr;
  
    uint32_t size = response.ByteSize();
    pkt = new uint8_t[size];

    uint32_t nSize = htonl(size);
    status = evbuffer_add(output, &nSize, sizeof(uint32_t));
    ArrayOutputStream aos(pkt, size);
    CodedOutputStream coded_output(&aos);
    response.SerializeToCodedStream(&coded_output);

    status = evbuffer_add(output, pkt, size);

    delete pkt;
  }
}
