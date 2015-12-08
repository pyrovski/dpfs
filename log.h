#ifndef LOG_HH
#define LOG_HH

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

#define logmsg(log, format...) \
  (log).print(__FILE__, __LINE__, "", format)

#define prefixmsg(log, prefix, format...) \
  (log).print(__FILE__, __LINE__, prefix, format)

#ifdef DEBUG
#define dbgmsg(log, format...) do { \
    (log).print(__FILE__, __LINE__, "debug", format); (log).flush();	\
      } while (0)
#else
#define dbgmsg(log...) do {} while(0)
#endif

//!@todo if not daemonized, also print to stderr
#define errmsg(log, format...) \
  (log).print(__FILE__, __LINE__, "error", format)

#define failmsg(log, format...)				\
  (log).fail(__FILE__, __LINE__, format)

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
