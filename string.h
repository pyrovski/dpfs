#ifndef STRING_H
#define STRING_H

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <uuid/uuid.h>

// trim from start
static inline std::string &ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
  return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
  return ltrim(rtrim(s));
}

static inline std::string strToLower(const std::string & str){
  std::string strLower = str;
  std::transform(strLower.begin(), strLower.end(), strLower.begin(), tolower);
  return strLower;
}

int strSplit(const std::string &str, const char split, std::string & lhs,
	     std::string & rhs);

std::string to_string(const uuid_t & uuid);

#endif
