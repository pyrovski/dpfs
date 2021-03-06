#include "string.h"
#include <uuid/uuid.h>

using namespace std;

int strSplit(const string &str, const char split, string & lhs, string & rhs){
  size_t offset = str.find(split);
  if(offset == string::npos){
    lhs = str;
    return -1;
  }
  
  lhs = str.substr(0, offset);
  rhs = str.substr(offset + 1, string::npos);
  return 0;
}

string to_string(const uuid_t & uuid){
  string result;
  char buf[37];
  uuid_unparse(uuid, buf);
  result.assign(buf);
  return result;
}
