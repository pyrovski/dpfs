#ifndef MONMANAGER_H
#define MONMANAGER_H

#include <string>
#include <thread>
#include <unordered_set>

#include "MonClient.h"

class MonManager {
 public:
  /*!@param monitors comma-separated list of monitors in host[:port] format
   */
  MonManager(const std::string * monitors, const uuid_t fsid);
  ~MonManager();

  int start();
  int stop();
  int request();
  const uuid_t & getFSID() const;
  bool isRunning();
  struct bufferevent * registerClient(MonClient *);
  void unregisterClient(MonClient *);
  void timeout(double timeoutSeconds = defaultMonTimeoutSeconds);
  int validateFSID(); // wait for clients to validate FSID

 private:
  struct event_base * base;
  std::unordered_set<MonClient *> clients;
  struct event * evtimeout;

  //!@todo
  std::mutex theMutex; // protects fsid, cv, tid, running
  std::condition_variable cv;
  uuid_t fsid;
  bool running;
  std::thread runThread;
  void run();
};

#endif
