#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <uuid/uuid.h>

#include "log.h"
#include "event.h"

std::string buildConfPath(const char * path = NULL, const char * name = NULL);

int loadOrCreateFSID(uuid_t &fsid, const char * path = 0);

void daemonize(log_t &log);

int set_tcp_no_delay(evutil_socket_t fd);

int strSplit(const std::string &str, const char split, std::string & lhs, std::string & rhs);

#endif
