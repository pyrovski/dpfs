#include <string>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "log.h"
#include "SysLock.h"
#include "defaults.h"
#include "event.h"

using namespace std;

void buildConfPaths(vector<string> &result, bool includeSysPaths, const char * name){

  result.push_back((string)"./" + (string)name);

  char *home = getenv("HOME");
  if(home)
    result.push_back((string)home + (string)"/.config/" +
			     (string)defaultConfDir + (string)"/" + (string)name);

  if(includeSysPaths)
    for(int i = 0; i < sizeof(defaultConfPaths) / sizeof(defaultConfPaths[0]); i++){
      string path(defaultConfPaths[i]);
      path += "/"; //!@todo platform-specific
      path += name;
      result.push_back(path);
    }
}

/*! If fsid is NULL, create a new fsid if none exist or load the first
  fsid found. If fsid is not NULL, attempt to load it and create it if
  it does not exist.
 */
//!@todo split config dir into function
int loadOrCreateFSID(uuid_t &fsid, const char * path){
  log_t log("/dev/stdout");
  int status;
  int result = 0;

  bool createOrLoadFirst = false;
  bool specific = false;
  
  if(uuid_is_null(fsid)){
    createOrLoadFirst = true;
  } else { // look for a specific UUID
    specific = true;
  }

  assert(specific ^ createOrLoadFirst);

  string defaultPath;
  
  if(!path){
    char *home = getenv("HOME");
    if(home)
      defaultPath = home;
    else
      defaultPath = ".";

    defaultPath += "/.config/";
    defaultPath += defaultConfDir;
    path = defaultPath.c_str();
  }
  
  DIR * dir;
  do {
    errno = 0;
    dir = opendir(path);
    if(!dir){
      if(errno == ENOENT){
	errno = 0;
	status = mkdir(path, S_IRWXU);
	if(status){
	  errmsg(log, "failed to create %s: %s", path, strerror(errno));
	  return -1;
	}
	continue;
      } else {
	errmsg(log, "failed to open %s: %s", path, strerror(errno));
	return -1;
      }
    }
  } while(!dir && errno != ENOENT);

  SysLock sysLock;
  sysLock.lock();
  
  struct dirent * entry;
  do {
    errno = 0;
    entry = readdir(dir);
    if(!entry){
      if(errno){
	errmsg(log, "readdir failed: %s", strerror(errno));
	result = -1;
	goto fail;
      }
      break;
    }
#ifndef _DIRENT_HAVE_D_TYPE
#error expected D_TYPE
#endif

    if(entry->d_type != DT_DIR)
      continue;

    if(strlen(entry->d_name) != 36)
      continue;

    uuid_t dirUUID;
    status = uuid_parse(entry->d_name, dirUUID);
    if(status){
      dbgmsg(log, "failed to parse %s as UUID", entry->d_name);
      continue;
    }
  
    // have parsed UUID
    if(specific && !uuid_compare(fsid, dirUUID))
      break;
    else if(createOrLoadFirst)
      break;
  } while(entry);

  if(entry){ // found something
    //!@todo validate directory contents, etc.
  } else if(specific){ // didn't find specific fsid or didn't find any fsid
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
  } else if(createOrLoadFirst){
    uuid_generate(fsid);
    //!@todo create directory
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
