#include <utility>
#include <assert.h>
#include "MonManager.h"

using namespace std;

MonManager::MonManager(const log_t & log, string monitors):
  log(log)
{
  //!@todo
}

/*
void MonManager::registerThread(){
  unique_lock<mutex> lock(theMutex);
  tid = gettid();
  lock.unlock();
}
*/

bool MonManager::isRunning(){
  bool result;
  unique_lock<mutex> lock(theMutex);
  result = running;
  lock.unlock();
  return result;
}

//!@todo fix
int MonManager::stop(){
  //pid_t registeredTID;
  unique_lock<mutex> lock(theMutex);
  running = false;
  //registeredTID = tid;
  lock.unlock();
  //if(gettid() != registeredTID){
    //!@todo send signal to registeredTID
    //int status = pthread_kill(threadID.native_handle(), SIGTERM);
  //}
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
