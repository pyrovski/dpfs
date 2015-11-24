#include <iostream>
#include <chrono>
#include <iomanip>

#include <stdlib.h>
#include <math.h>
#include <stdarg.h>

#include "log.h"

using namespace std;

log_t::log_t(const char * file){
  if(!file){
    logFile = stdout;
    return;
  }

  logFile = fopen(file, "a");

  if(!logFile){
    cerr << "failed to open log file " << file << endl;
    exit(1);
  }
  
  fflush(logFile);
}

log_t::~log_t(){
  fclose(logFile);
}

void log_t::print(const char * file, int line, const char * prefix,
		  const char * str, ...) const {
  va_list vl;
  va_start(vl, str);
  vprint(file, line, prefix, str, vl);
  va_end(vl);
}

void log_t::vprint(const char * file, int line, const char * prefix,
		    const char * str, va_list vl) const {
  chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
  chrono::high_resolution_clock::duration duration = t1.time_since_epoch();
  auto numerator = chrono::high_resolution_clock::period::num;
  auto denominator = chrono::high_resolution_clock::period::den;

  fprintf(logFile, "%lu.%06u:%s:%d:%s: ",  
	  duration.count() * numerator / denominator,
	  (int) (fmod(duration.count() * numerator, denominator)/1e3f),
	  file, line,
	  prefix);
  vfprintf(logFile, str, vl);
  fprintf(logFile, "\n");
}
