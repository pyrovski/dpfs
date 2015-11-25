#ifndef UTIL_H
#define UTIL_H

#include <uuid/uuid.h>

#include "log.h"

int loadOrCreateFSID(uuid_t &fsid, const char * path = "/var/lib/dpfs");

void daemonize(log_t &log);

#endif
