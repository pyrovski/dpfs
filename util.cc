#include <string>

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <google/protobuf/io/coded_stream.h>
//#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "util.h"
#include "log.h"
#include "SysLock.h"
#include "defaults.h"
#include "event.h"

using namespace std;
using namespace google::protobuf::io;

string buildConfPath(const char * path, const char * name){
  string result;

  if(path){
    result = path;
  } else {
    char *home = getenv("HOME");
    if(home){
      result =
	(string)home + (string)"/.config/" +
	(string)defaultConfDir;
    } else
      result = ".";
  }
  if(name)
    result += (string)"/" + (string)name;
  return result;
}

/*! If fsid is NULL, create a new fsid if none exist or load the first
  fsid found. If fsid is not NULL, attempt to load it and create it if
  it does not exist.
 */
int loadOrCreateFSID(const log_t & log, uuid_t &fsid, const char * path){
  int status;
  int result = 0;
  
  if(uuid_is_null(fsid)){
    errmsg(log, "Require specific non-null UUID");
    return -1;
  }
  
  // look for a specific UUID

  string pathStr;
  
  if(path)
    pathStr = path;
  else
    pathStr = buildConfPath();

  path = pathStr.c_str();
  DIR * dir = openCreateDir(log, path);

  SysLock sysLock;
  sysLock.lock();

  auto findUUID = [&](struct dirent * entry)->int {
#ifndef _DIRENT_HAVE_D_TYPE
#error expected D_TYPE
#endif
    if(entry->d_type != DT_DIR)
      return 1;

    if(strlen(entry->d_name) != 36)
      return 1;

    uuid_t dirUUID;
    status = uuid_parse(entry->d_name, dirUUID);
    if(status){
      dbgmsg(log, "failed to parse %s as UUID", entry->d_name);
      return 1;
    }
  
    // have parsed UUID
    if(!uuid_compare(fsid, dirUUID))
      return 0;
    return 1;
  };
  
  status = iterateDir(log, dir, findUUID);

  if(!status){ // found something
    //!@todo validate directory contents, etc.
  } else { // didn't find specific fsid or didn't find any fsid
    string fsidPath = path;
    fsidPath += "/";
    char fsidStr[37];
    uuid_unparse(fsid, fsidStr);
    fsidPath += fsidStr;
    status = mkdir(fsidPath.c_str(), S_IRWXU | S_IRWXG);
    if(status){
      errmsg(log, "failed to create %s: %s", fsidPath.c_str(), strerror(errno));
      result = -1;
      goto fail;
    }
  }
 fail:
  sysLock.unlock();
  closedir(dir);
  return result;
}

void daemonize(log_t &log){
  pid_t pid = fork();
  if(!pid){
    // child
    pid = setsid();
    if(pid == -1)
      failmsg(log, "failed setsid.");

    dbgmsg(log, "monitor forked");
    {
      const int fd = open ("/dev/null", O_RDWR, 0);
      dup2 (fd, STDIN_FILENO);
      dup2 (fd, STDOUT_FILENO);
      dup2 (fd, STDERR_FILENO);
      close (fd);
    }
  } else if(pid > 0){
    // parent
    exit(0);
  } else if(pid < 0)
    failmsg(log, "failed to fork.");
}

int set_tcp_no_delay(evutil_socket_t fd)
{
  int one = 1;
  return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		    &one, sizeof one);
}

int strSplit(const string &str, const char split, string & lhs, string & rhs){
  size_t offset = str.find(split);
  if(offset == string::npos){
    lhs = str;
    return -1;
  }
  
  lhs = str.substr(0, offset);
  rhs = str.substr(offset + 1, string::npos);
  return 0;
}

/*!
  @param dir open directory
  @param func function returns zero on success
  @return -1 on error, 0 if func returned 0, 1 if all entries examined
 */
int iterateDir(const log_t & log, DIR * dir,
	       const function< int(struct dirent *) >& func)
{
  struct dirent * entry = NULL;
  do {
    errno = 0;
    entry = readdir(dir);
    if(!entry){
      if(errno){
	errmsg(log, "readdir failed: %s", strerror(errno));
	return -1;
      }
      break;
    }
    int status = func(entry);
    if(status)
      continue;
    else
      return 0;
  } while(entry);
  return 1;
}

DIR * openCreateDir(const log_t & log, const char * path){
  DIR * dir = NULL;
  do {
    errno = 0;
    dir = opendir(path);
    if(!dir){
      if(errno == ENOENT){
	errno = 0;
	int status = mkdir(path, S_IRWXU);
	if(status){
	  errmsg(log, "failed to create %s: %s", path, strerror(errno));
	  return NULL;
	}
	continue;
      } else {
	errmsg(log, "failed to open %s: %s", path, strerror(errno));
	return NULL;
      }
    }
  } while(!dir && errno != ENOENT);
  return dir;
}

int scanFSIDs(const log_t & log, unordered_set<uuid_s> &uuids){
  string path = buildConfPath();
  DIR * dir = NULL;
  errno = 0;
  dir = opendir(path.c_str());
  if(!dir){
    errmsg(log, "failed to open %s: %s", path.c_str(), strerror(errno));
    return -1;
  }
  
  auto scanForUUIDs = [&](struct dirent * entry){
    if(entry->d_type != DT_DIR)
      return 1;
    
    if(strlen(entry->d_name) != 36)
      return 1;
    
    uuid_s dirUUID;
    int status = uuid_parse(entry->d_name, dirUUID.uuid);
    if(status){
      dbgmsg(log, "failed to parse %s as UUID", entry->d_name);
      return 1;
    }
    uuids.insert(dirUUID);
    // don't stop iterateDir loop prematurely
    return 1;
  };
  int status = iterateDir(log, dir, scanForUUIDs);
  closedir(dir);
  return 0;
}

int createOSD(const log_t & log, const uuid_s & fsid, const char *dataPath){
  int result = 0;
  SysLock sysLock;
  sysLock.lock();

  string path = buildConfPath();
  char fsidStr[37];
  uuid_unparse(fsid.uuid, fsidStr);
  path += (string)"/" + fsidStr;
  int status = mkdir(path.c_str(), S_IRWXU);
  if(status && errno != EEXIST){
    errmsg(log, "failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  path += "/OSD/";
  status = mkdir(path.c_str(), S_IRWXU);
  if(status && errno != EEXIST){
    errmsg(log, "failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  path += to_string(nextInt(log, path.c_str()));
  status = mkdir(path.c_str(), S_IRWXU);
  if(status){
    errmsg(log, "failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  path += "/data";
  if(dataPath)
    status = symlink(dataPath, path.c_str());
  else
    status = mkdir(path.c_str(), S_IRWXU);

  if(status){
    errmsg(log, "failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

 cleanup:
  sysLock.unlock();
  return result;
}

int nextInt(const log_t & log, const char * path){
  DIR * dir = opendir(path);
  int highest = INT_MIN;

  auto maxInt = [&](struct dirent * entry)->int{
    if(entry->d_type != DT_DIR)
      return 1;

    //!@todo check for non-digit characters in name. If found, return 1.
    
    errno = 0;
    char * endptr = NULL;
    long result = strtol(entry->d_name, &endptr, 10);
    if(!errno && endptr != entry->d_name)
      highest = max(highest, (int)result);

    return 1;
  };
  
  int status = iterateDir(log, dir, maxInt);
  closedir(dir);
  return max(highest + 1, 0);
}

/* What information do we need here? OSDs Can't be created until the
   FSID exists.
 */
int createFS(const log_t & log, uuid_s & fsid, const FSOptions::FSOptions & fsOptions){
  int result = 0;
  int status;
  if(uuid_is_null(fsid.uuid))
    uuid_generate(fsid.uuid);
  // create directory
  status = loadOrCreateFSID(log, fsid.uuid);
  if(status){
    printf("FSID creation failed: %d", status);
    return -1;
  }
  
  // save fsOptions to file in FS dir
  evbuffer * buf = evbuffer_new();
  char fsidStr[37];
  uuid_unparse(fsid.uuid, fsidStr);
  status = message_to_evbuffer(fsOptions, buf, false);

  string path = buildConfPath(NULL, fsidStr) + "/init";
  
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
  if(fd < 0){
    errmsg(log, "Failed to open %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  status = evbuffer_write(buf, fd);
  if(status == -1){
    errmsg(log, "Failed to write init info");
    result = -1;
    goto cleanup;
  }

 cleanup:
  evbuffer_free(buf);
  return result;
}

int message_to_evbuffer(const ::google::protobuf::MessageLite &msg, evbuffer * output, bool prefixSize){
  int status;
  uint32_t size = msg.ByteSize();
  if(prefixSize){
    uint32_t nSize = htonl(size);
    status = evbuffer_add(output, &nSize, sizeof(uint32_t));
  }
  //pkt = new uint8_t[size];
  struct evbuffer_iovec iovec;
  status = evbuffer_reserve_space(output, size, &iovec, 1);
  if(status == -1){
    printf("Failed to reserve %d bytes of buffer space", size);
    return -1;
  }
    
  ArrayOutputStream aos(iovec.iov_base, size);
  CodedOutputStream coded_output(&aos);
  msg.SerializeToCodedStream(&coded_output);
  
  //status = evbuffer_add(output, pkt, size);
  status = evbuffer_commit_space(output, &iovec, 1);
  if(status == -1){
    printf("Failed to commit %d bytes of buffer space", size);
    return -1;
  }
  return 0;
}
