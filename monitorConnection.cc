#include "monitorConnection.h"

void monitorConnection::processInput(struct evbuffer * input){
  if(connection->state == monitorConnStateDefault){
    uint32_t nSize;

    int status = evbuffer_remove(input, &nSize, sizeof(nSize));
    if(status == -1){
      dbgmsg(log, "read error: %d", status);
      return;
    } else
      dbgmsg(log, "read %d bytes on socket %d",
	     status, connection->socket);

    if(status != sizeof(nSize)){
      errmsg(log, "read %d/%d on socket %d",
	     status, sizeof(nSize), connection->socket);
      return;
    }    
    connection->incomingSize = ntohl(nSize);

    if(connection->incomingSize == 0)
      errmsg(log, "expected nonzero size");
    
    connection->state = monitorConnStateReceivedSize;
  } else if(connection->state == monitorConnStateReceivedSize){
    struct evbuffer * output = bufferevent_get_output(bev);
    //!@todo read request, send response
    mon::Query query;

    uint8_t * pkt = new uint8_t[connection->incomingSize];
    int status =
      evbuffer_remove(input, pkt, connection->incomingSize);
    if(status == -1)
      dbgmsg(log, "read error: %d: %s", errno, strerror(errno));
    else
      dbgmsg(log, "read %d bytes on socket %d", status, connection->socket);
    
    //!@todo build query, get time from query, drain input
    delete pkt;
    
    mon::Response response;
    mon::Response::Mons mons;
    
    // get list of addresses on monitor host.

    //!@todo If client is connected from localhost, don't filter out
    //!loopback addresses from getaddrinfo().
    string portStr = to_string(parent->getPort());
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *addressInfo = NULL;
    status = getaddrinfo(NULL, portStr.c_str(), &hints, &addressInfo);
    if(status != 0){
      errmsg(log, "getaddrinfo failed: %d", status);
      return;
    }
    
    for(; addressInfo; addressInfo = addressInfo->ai_next){
      netAddress::Address monAddress;
      monAddress.set_port(parent->getPort());
      //!@todo get ipv4/ipv6, set address in monAddress, add monAddress
      //!to mons, add mons to response.
      if(addressInfo->ai_family == AF_INET){
	netAddress::Address_IPV4Address monAddress4;
	monAddress4.set_address(ntohl(*(uint32_t*)addressInfo->ai_addr));
	*monAddress.mutable_address4() = monAddress4;
      } else if(addressInfo->ai_family == AF_INET6){
	netAddress::Address_IPV6Address monAddress6;
	// addressInfo->ai_addr is in network byte order
	uint32_t addressHigh, addressLow;
	addressLow = ntohl(*(uint32_t*)addressInfo->ai_addr);
	addressHigh = ntohl(*(((uint32_t*)addressInfo->ai_addr) + 1));
	if(bigEndian()){
	  swap(addressLow, addressHigh);
	}
	monAddress6.set_addresshigh(addressHigh);
	monAddress6.set_addresslow(addressLow);
	*monAddress.mutable_address6() = monAddress6;
      } else {
	errmsg(log, "unknown address type: %d", addressInfo->ai_family);
	continue;
      }
    
      *mons.add_address() = monAddress;
    }
    *response.mutable_mons() = mons;

    //!@todo
    mon::Response::MDSs mdss;
    mon::Response::OSDs osds;
    *response.mutable_osds() = osds;
    *response.mutable_mdss() = mdss;
  
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
