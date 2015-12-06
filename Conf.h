#ifndef CONF_H
#define CONF_H

#include <unordered_map>
#include <string>

#include "log.h"

class Conf {
 public:
  Conf(const log_t * log, const char * file = 0);
  const std::string * get(const std::string & key) const;
  bool hasKey(const std::string & key) const;
  int load();
  
 private:
  std::unordered_map<std::string, std::string> map;
  const log_t * log;
  const char * confFile;
};

#endif
