#ifndef UTIL_H
#define UTIL_H

#include <functional>
#include <string>
#include <unordered_set>

#include <sys/types.h>
#include <dirent.h>
#include "uuid_s.h"
#include "log.h"
#include "event.h"
#include "FSOptions.pb.h"

std::string buildConfPath(const char * path = NULL, const char * name = NULL);

int loadOrCreateFSID(const log_t & log, uuid_t &fsid, const char * path = 0);

void daemonize(const log_t &log);

int set_tcp_no_delay(evutil_socket_t fd);

int strSplit(const std::string &str, const char split, std::string & lhs, std::string & rhs);

int iterateDir(const log_t & log, DIR * dir,
	       const std::function< int(struct dirent *) > & func);

DIR * openCreateDir(const log_t & log, const char * path);

int scanFSIDs(const log_t & log, std::unordered_set<uuid_s> &uuids);

int createOSD(const log_t & log, const uuid_s & fsid, const char *dataPath);

int nextInt(const log_t & log, const char * path);

int createFS(const log_t & log, uuid_s & fsid, const FSOptions::FSOptions & fsOptions);

int size_prefix_message_to_evbuffer(const ::google::protobuf::MessageLite &msg, evbuffer * output);

#endif
