#include <sstream>
#include <utility>
#include <assert.h>
#include "MonManager.h"
#include "util.h"

using namespace std;

//!@todo (general) check if calling thread differs from run thread


//!
static void timeoutCB(evutil_socket_t fd, short flags, void * arg){
  MonManager * parent = (MonManager *) arg;
  dbgmsg(parent->getLog(), "timeout");
  parent->timeout();
}

//!
MonManager::MonManager(const log_t & log, const string *monitors):
  log(log)
{
  base = event_base_new();
  if(monitors)
    dbgmsg(log, "monitors: %s", monitors->c_str());
  else
    failmsg(log, "no monitors!");

  string token;
  istringstream ss(*monitors);
  while(getline(ss, token, ',')){
    istringstream hostSS(token);
    string addressStr, portStr;
    if(strSplit(token, ':', addressStr, portStr)){
      if(!addressStr.length())
	continue;
    }
    uint16_t port;
    if(portStr.length())
      port = stoi(portStr);
    else
      port = defaultMonPort;
    clients.insert(new MonClient(log, *this, addressStr.c_str(), port));
  }
  evtimeout = evtimer_new(base, timeoutCB, this);
}

MonManager::~MonManager(){
  event_free(evtimeout);
  event_base_free(base);
}

bool MonManager::isRunning(){
  bool result;
  unique_lock<mutex> lock(theMutex);
  result = running;
  lock.unlock();
  return result;
}

const log_t & MonManager::getLog() const {
  return log;
}

void MonManager::timeout(double timeoutSeconds){
  struct timeval timeout = to_tv(timeoutSeconds);
  evtimer_add(evtimeout, &timeout);
  for(auto client : clients)
    client->request();
}

void MonManager::run(){
  for(auto client : clients)
    client->connect();

  timeout(0);
  event_base_dispatch(base);
}

int MonManager::start(){
  /*!@todo create thread, store new thread id, set running=true if successful
   */
  unique_lock<mutex> lock(theMutex);
  if(!running){
    runThread = std::thread( [=] { run(); } );
    running = true;
  }
  lock.unlock();
  
  return 0;
}

//!@todo fix
int MonManager::stop(){
  unique_lock<mutex> lock(theMutex);
  running = false;
  lock.unlock();
}

/*! If client has retrieved FSID from monitor, return FSID to
  caller. Otherwise, wait for FSID from monitor first.
 */
int MonManager::getFSID(uuid_t &fsid){
  bool done = false;
  unique_lock<mutex> lock(theMutex);
  if(fsid_set){
    uuid_copy(fsid, this->fsid);
    done = true;
  }
  if(done){
    lock.unlock();
    return 0;
  }
  
  while(!fsid_set)
    cv.wait(lock);

  lock.unlock();
  uuid_copy(fsid, this->fsid);
  
  return 0;
}

int MonManager::setFSID(const uuid_t &fsid){
  unique_lock<mutex> lock(theMutex);
  uuid_copy(this->fsid, fsid);
  fsid_set = true;
  lock.unlock();
  cv.notify_all();
}

struct bufferevent * MonManager::registerClient(MonClient * client){
  assert(client);
  clients.insert(client);
  return bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
}

void MonManager::unregisterClient(MonClient * client){
  clients.erase(client);
}
