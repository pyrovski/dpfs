#include <fstream>
#include <vector>
#include <string>
#include <errno.h>
#include <string.h>
#include "Conf.h"
#include "string.h"
#include "defaults.h"
#include "util.h"

using namespace std;

Conf::Conf(const log_t * log, const char * confFile): confFile(confFile), log(log){
}

/*! Read <key> = <value> lines from file, add to map
 */
int Conf::load(){

  ifstream file;

  vector<string> fileCandidates;
  if(confFile)
    fileCandidates.push_back(confFile);
  else
    buildConfPaths(fileCandidates, true, defaultConfFile);

  for(int i = 0; i < fileCandidates.size(); i++){
    dbgmsg(*log, "attempting conf candidate %s", fileCandidates[i].c_str());
    file.open(fileCandidates[i].c_str());
    if(!file.is_open()){
      dbgmsg(*log, "failed to load config file %s:%s", fileCandidates[i].c_str(), strerror(errno));
      continue;
    }
  }
  if(!file.is_open()){
    errmsg(*log, "failed to load any config file");
    return -1;
  }

  for(string line; file.good(); getline(file, line)){
    dbgmsg(*log, "read: \"\" from %s", line.c_str(), confFile);
    size_t offset = line.find("=");
    if(offset == string::npos)
      continue;

    string key = line.substr(0, offset),
      value = line.substr(offset + 1, string::npos);
    key = trim(key);
    //!@todo convert key to lower case
    value = trim(value);
    if(key.length() > 0 && value.length() > 0){
      dbgmsg(*log, "adding to map: %s = %s", key.c_str(), value.c_str());
      map[key] = value;
    }
  }

  file.close();
  
  return 0;
}  

const string * Conf::get(const string & key) const {
  //!@todo convert key to lower case
  auto it = map.find(key);
  if(it != map.end())
    return &it->second;
  else
    return NULL;
}

bool Conf::hasKey(const std::string & key) const {
  //!@todo convert key to lower case
  auto it = map.find(key);
  return (it != map.end());
}
