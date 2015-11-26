#ifndef SYSLOCK_H
#define SYSLOCK_H

/* protect against race conditions between daemons on a single host */
class SysLock {
 public:
  SysLock();

  int tryLock() const;
  int lock() const;
  int unlock() const;
  
 private:
  int forceLock() const;
  int writePid(int fd) const;
};

#endif
