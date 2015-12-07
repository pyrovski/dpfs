#ifndef MONMANAGER_H
#define MONMANAGER_H

#include <unordered_set>

#include "MonClient.h"

class MonManager {
 public:
  /*!@param monitors comma-separated list of monitors in host[:port] format
   */
  MonManager(const log_t & log, string monitors);

  int start();
  int stop();
  int request();
  int getFSID(uuid_t &fsid);
  bool isRunning();
  struct bufferevent * registerClient(MonClient *);

 private:
  struct event_base * base;
  std::unordered_map<MonClient *> clients;

  int setFSID(const uuid_t &fsid);
  std::mutex theMutex; // protects fsid, fsid_set, cv, tid, stop
  std::condition_variable cv;
  uuid_t fsid;
  bool fsid_set;
  //pid_t tid;
  bool running;

  const log_t & log;
};

#endif
