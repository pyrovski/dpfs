#ifndef LOG_HH
#define LOG_HH

#include <fstream>

class log_t {
public:
  log_t(const char * file = "/tmp/dpfs.log");
  ~log_t();

  void print(const char *);

private:
  std::fstream log_fstream;
};

#endif
