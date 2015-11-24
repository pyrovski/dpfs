#ifndef LOG_HH
#define LOG_HH

#include <fstream>

#include <unistd.h>
#include <stdarg.h>

class log_t {
public:
  log_t(const char * file = 0);
  ~log_t();

  void print(const char *);
  void printf(const char * format, ...);
  void error(const char *, ...);
  void verr(const char *, va_list vl);
  void fail(const char *, ...);
  void dbg(const char *, ...);
  void vdbg(const char *, va_list vl);
  void flush();

private:
  FILE * logFile;
  void print_(const char *prefix, const char *, ...);
  void vprint_(const char *prefix, const char *, va_list vl);
};

inline void log_t::flush(){
  fflush(logFile);
}

inline void log_t::print(const char * str){
  print_("", str);
}

inline void log_t::printf(const char * format, ...){
  va_list vl;
  va_start(vl, format);
  vprint_("", format, vl);
  va_end(vl);
}

inline void log_t::error(const char * str, ...){
  va_list vl;
  va_start(vl, str);
  vprint_("error", str, vl);
  va_end(vl);
}

inline void log_t::fail(const char * str, ...){
  va_list vl;
  va_start(vl, str);
  vprint_("fatal", str, vl);
  va_end(vl);
  fflush(logFile);
  exit(1);
}

inline void log_t::dbg(const char * str, ...){
#ifdef DEBUG
  va_list vl;
  va_start(vl, str);
  vprint_("debug", str, vl);
  va_end(vl);
#endif
}

inline void log_t::vdbg(const char * str, va_list vl){
#ifdef DEBUG
  vprint_("debug", str, vl);
#endif
}

inline void log_t::verr(const char * str, va_list vl){
  vprint_("error", str, vl);
}

#endif
