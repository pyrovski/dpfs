#ifndef EVENT_H
#define EVENT_H
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

typedef void (eventCB)(evutil_socket_t, short flags, void * arg);

#endif
