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
int loadOrCreateFSID(uuid_t &fsid, const char * path){
  int status;
  int result = 0;
  
  if(uuid_is_null(fsid)){
    errmsg("Require specific non-null UUID");
    return -1;
  }
  
  // look for a specific UUID

  string pathStr;
  
  if(path)
    pathStr = path;
  else
    pathStr = buildConfPath();

  path = pathStr.c_str();
  DIR * dir = openCreateDir(path);

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
      dbgmsg("failed to parse %s as UUID", entry->d_name);
      return 1;
    }
  
    // have parsed UUID
    if(!uuid_compare(fsid, dirUUID))
      return 0;
    return 1;
  };
  
  status = iterateDir(dir, findUUID);

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
      errmsg("failed to create %s: %s", fsidPath.c_str(), strerror(errno));
      result = -1;
      goto fail;
    }
  }
 fail:
  sysLock.unlock();
  closedir(dir);
  return result;
}

void daemonize(){
  pid_t pid = fork();
  if(!pid){
    // child
    pid = setsid();
    if(pid == -1)
      failmsg("failed setsid.");

    dbgmsg("monitor forked");
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
    failmsg("failed to fork.");
}

int set_tcp_no_delay(evutil_socket_t fd)
{
  int one = 1;
  return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		    &one, sizeof one);
}

/*!
  @param dir open directory
  @param func function returns zero on success
  @return -1 on error, 0 if func returned 0, 1 if all entries examined
 */
int iterateDir(DIR * dir,
	       const function< int(struct dirent *) >& func)
{
  struct dirent * entry = NULL;
  do {
    errno = 0;
    entry = readdir(dir);
    if(!entry){
      if(errno){
	errmsg("readdir failed: %s", strerror(errno));
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

DIR * openCreateDir(const char * path){
  DIR * dir = NULL;
  do {
    errno = 0;
    dir = opendir(path);
    if(!dir){
      if(errno == ENOENT){
	errno = 0;
	int status = mkdir(path, S_IRWXU);
	if(status){
	  errmsg("failed to create %s: %s", path, strerror(errno));
	  return NULL;
	}
	continue;
      } else {
	errmsg("failed to open %s: %s", path, strerror(errno));
	return NULL;
      }
    }
  } while(!dir && errno != ENOENT);
  return dir;
}

int scanFSIDs(unordered_set<uuid_s> &uuids){
  string path = buildConfPath();
  DIR * dir = NULL;
  errno = 0;
  dir = opendir(path.c_str());
  if(!dir){
    errmsg("failed to open %s: %s", path.c_str(), strerror(errno));
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
      dbgmsg("failed to parse %s as UUID", entry->d_name);
      return 1;
    }
    uuids.insert(dirUUID);
    // don't stop iterateDir loop prematurely
    return 1;
  };
  int status = iterateDir(dir, scanForUUIDs);
  //!@todo check error
  closedir(dir);
  return 0;
}

int createOSD(const uuid_s & fsid, const char *dataPath){
  int result = 0;
  SysLock sysLock;
  sysLock.lock();

  string path = buildConfPath();
  char fsidStr[37];
  uuid_unparse(fsid.uuid, fsidStr);
  path += (string)"/" + fsidStr;
  int status = mkdir(path.c_str(), S_IRWXU);
  if(status && errno != EEXIST){
    errmsg("failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  path += "/OSD/";
  status = mkdir(path.c_str(), S_IRWXU);
  if(status && errno != EEXIST){
    errmsg("failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  path += to_string(nextInt(path.c_str()));
  status = mkdir(path.c_str(), S_IRWXU);
  if(status){
    errmsg("failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  path += "/data";
  if(dataPath)
    status = symlink(dataPath, path.c_str());
  else
    status = mkdir(path.c_str(), S_IRWXU);

  if(status){
    errmsg("failed to create %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

 cleanup:
  sysLock.unlock();
  return result;
}

int nextInt(const char * path){
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
  
  int status = iterateDir(dir, maxInt);
  //!@todo check error
  closedir(dir);
  return max(highest + 1, 0);
}

/* What information do we need here? OSDs Can't be created until the
   FSID exists.
 */
int createFS(uuid_s & fsid, const FSOptions::FSOptions & fsOptions){
  int result = 0;
  int status;

  if(fsOptions.pgs() <= 0){
    errmsg("Must supply a positive OSD count");
    return -1;
  }
  
  if(uuid_is_null(fsid.uuid))
    uuid_generate(fsid.uuid);
  // create directory
  status = loadOrCreateFSID(fsid.uuid);
  if(status){
    errmsg("FSID creation failed: %d", status);
    return -1;
  }
  
  // save fsOptions to file in FS dir
  evbuffer * buf = evbuffer_new();
  char fsidStr[37];
  uuid_unparse(fsid.uuid, fsidStr);
  status = message_to_evbuffer(fsOptions, buf);

  string path = buildConfPath(NULL, fsidStr) + "/" + defaultFSInitFile;
  
  int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
  if(fd < 0){
    errmsg("Failed to open %s: %s", path.c_str(), strerror(errno));
    result = -1;
    goto cleanup;
  }

  status = evbuffer_write(buf, fd);
  if(status == -1){
    errmsg("Failed to write init info");
    result = -1;
    goto cleanup;
  }

 cleanup:
  if(fd >= 0)
    close(fd);
  evbuffer_free(buf);
  return result;
}

int message_to_evbuffer(const ::google::protobuf::MessageLite &msg,
			evbuffer * output, bool prefixSize){
  int status;
  uint32_t size = msg.ByteSize();
  dbgmsg("outgoing msg size: %d", size);
  if(prefixSize){
    uint32_t nSize = htonl(size);
    status = evbuffer_add(output, &nSize, sizeof(uint32_t));
  }
  //pkt = new uint8_t[size];
  struct evbuffer_iovec iovec;
  status = evbuffer_reserve_space(output, size, &iovec, 1);
  if(status == -1){
    errmsg("Failed to reserve %d bytes of buffer space", size);
    return -1;
  }
  if(iovec.iov_len > size){
    // evbuffer_reserve_space() may return more space than requested.
    dbgmsg("truncating iovec from %d to %d", iovec.iov_len, size);
    iovec.iov_len = size;
  }

  ArrayOutputStream aos(iovec.iov_base, size);
  CodedOutputStream coded_output(&aos);
  msg.SerializeToCodedStream(&coded_output);
  
  //status = evbuffer_add(output, pkt, size);
  status = evbuffer_commit_space(output, &iovec, 1);
  if(status == -1){
    errmsg("Failed to commit %d bytes of buffer space", size);
    return -1;
  }
  return 0;
}


int evbuffer_to_message(evbuffer * input, ::google::protobuf::MessageLite &msg,
			bool prefixSize){
  uint32_t size = -1;
  uint32_t nSize;
  int status;
  if(prefixSize){
    status = evbuffer_remove(input, &nSize, sizeof(uint32_t));
    if(status != sizeof(uint32_t)){
      //!@todo errmsg
      return -1;
    }
    size = ntohl(nSize);
  }

  unsigned char * pkt = evbuffer_pullup(input, -1);
  if(!pkt){
    //!@todo errmsg, possibly attempt to drain
    return -1;
  }
  ArrayInputStream ais(pkt, size);
  CodedInputStream coded_input(&ais);
  CodedInputStream::Limit msgLimit = coded_input.PushLimit(size);
  
  //deserialize
  msg.ParseFromCodedStream(&coded_input);
  coded_input.PopLimit(msgLimit);

  evbuffer_drain(input, size);

  return 0;
}

