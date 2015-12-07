#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <string>
#include <uuid/uuid.h>

#include "log.h"
#include "event.h"

void buildConfPaths(std::vector<std::string> &result, bool includeSysPaths, const char * name);

int loadOrCreateFSID(uuid_t &fsid, const char * path = 0);

void daemonize(log_t &log);

int set_tcp_no_delay(evutil_socket_t fd);

#endif
