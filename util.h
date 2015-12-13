#ifndef UTIL_H
#define UTIL_H

#include <functional>
#include <string>
#include <unordered_set>

#include <sys/types.h>
#include <dirent.h>
#include "uuid_s.h"
#include "event.h"
#include "FSOptions.pb.h"

std::string buildConfPath(const char * path = NULL, const char * name = NULL);

int loadOrCreateFSID(uuid_t &fsid, const char * path = 0);

void daemonize();

int set_tcp_no_delay(evutil_socket_t fd);

int strSplit(const std::string &str, const char split, std::string & lhs, std::string & rhs);

int iterateDir(DIR * dir,
	       const std::function< int(struct dirent *) > & func);

DIR * openCreateDir(const char * path);

int scanFSIDs(std::unordered_set<uuid_s> &uuids);

int createOSD(const uuid_s & fsid, const char *dataPath);

int nextInt(const char * path);

int createFS(uuid_s & fsid, const FSOptions::FSOptions & fsOptions);

int message_to_evbuffer(const ::google::protobuf::MessageLite &msg,
			evbuffer * output, bool prefixSize=true);

int evbuffer_to_message(evbuffer * input, ::google::protobuf::MessageLite &msg,
			bool prefixSize=true);

#endif
