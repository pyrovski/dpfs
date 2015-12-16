#ifndef LOG_HH
#define LOG_HH

//!@todo why would we ever want more than one log per executable? */

#include <fstream>

#include <unistd.h>
#include <stdarg.h>

class log_t {
public:
  log_t(const char * file = 0);
  ~log_t();

  void fail(const char * file, int line, const char *, ...) const;
  void flush() const;
  void print(const char * file, int line, const char *prefix, const char *, ...) const;
  void vprint(const char * file, int line, const char *prefix,
	      const char *, va_list vl) const;

private:
  FILE * logFile;
};

extern log_t dpfsGlobalLog;

//!@Todo fix
#define logmsg(...) \
  (dpfsGlobalLog).print(__FILE__, __LINE__, "", __VA_ARGS__)

#define prefixmsg(prefix, ...) \
  (dpfsGlobalLog).print(__FILE__, __LINE__, prefix, __VA_ARGS__)

#ifdef DEBUG
#define dbgmsg(...) do { \
    (dpfsGlobalLog).print(__FILE__, __LINE__, "debug", __VA_ARGS__);\
    (dpfsGlobalLog).flush();					    \
      } while (0)
#else
#define dbgmsg(...) do {} while(0)
#endif

//!@todo if not daemonized, also print to stderr
#define errmsg(...) \
  (dpfsGlobalLog).print(__FILE__, __LINE__, "error", __VA_ARGS__)

#define failmsg(...)				\
  (dpfsGlobalLog).fail(__FILE__, __LINE__, __VA_ARGS__)

inline void log_t::flush() const {
  fflush(logFile);
}

inline void log_t::fail(const char * file, int line, const char * str, ...) const {
  //!@todo if not daemonized, also print to stderr
  va_list vl;
  va_start(vl, str);
  vprint(file, line, "fatal", str, vl);
  va_end(vl);
  fflush(logFile);
  exit(1);
}

#endif
