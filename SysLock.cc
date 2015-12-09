#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "defaults.h"
#include "SysLock.h"
#include "time.h"

SysLock::SysLock(){
}

int SysLock::writePid(int fd) const {
  pid_t pid = getpid();
  FILE * file = fdopen(fd, "w");
  if(!file){
    close(fd);
    return -1;
  }
  fprintf(file, "%d", pid);
  fclose(file);
  return 0;
}
  
int SysLock::tryLock() const {
  int fd = open(defaultLockFile, O_EXCL | O_CREAT | O_WRONLY,
		S_IRUSR | S_IWUSR);
  if(fd >= 0){
    return writePid(fd);
  } else
    return -1;
}

int SysLock::forceLock() const {
  int fd = open(defaultLockFile, O_CREAT | O_WRONLY | O_TRUNC,
		S_IRUSR | S_IWUSR);
  if(fd < 0)
    return -1;
  return writePid(fd);
}

// should not fail. Bounded timeout.
int SysLock::lock() const {
  int status;

  struct timeval initialTime;
  struct timeval now;
  gettimeofday(&initialTime, NULL);
  
  do {
    gettimeofday(&now, NULL);
    status = tryLock();
    if(status)
      usleep(1000);
  } while (status &&
	   tvDiff(now, initialTime) <= defaultSysLockTimeoutSeconds);
  if(status)
    forceLock();

  return 0;
}

int SysLock::unlock() const {
  int fd = open(defaultLockFile, O_RDONLY, 0);
  if(fd < 0){
    if(errno == ENOENT)
      return 0;
    return -1;
  }
  FILE * file = fdopen(fd, "r");
  if(!file)
    return -1;
  
  pid_t myPid, filePid;
  myPid = getpid();
  fscanf(file, "%d", &filePid);
  fclose(file);
  if(myPid == filePid)
    return unlink(defaultLockFile);
  
  return -1;
}
