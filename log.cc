#include <iostream>
#include <chrono>
#include <iomanip>

#include <stdlib.h>
#include <math.h>

#include "log.h"

using namespace std;

log_t::log_t(const char * file){
  log_fstream.open(file, ios_base::app);

  if(log_fstream.fail()){
    cerr << "failed to open log file " << file << endl;
    exit(1);
  }
  
  flush(log_fstream);
}

log_t::~log_t(){
  log_fstream.close();
}

void log_t::print(const char * str){
  
  chrono::high_resolution_clock::time_point t1 = chrono::high_resolution_clock::now();
  chrono::high_resolution_clock::duration duration = t1.time_since_epoch();
  auto numerator = chrono::high_resolution_clock::period::num;
  auto denominator = chrono::high_resolution_clock::period::den;
  log_fstream << 
    duration.count() * numerator / denominator << "." << setfill('0') <<
    setw(6) << (int) (fmod(duration.count() * numerator, denominator)/1e2f) <<
    ": "  << str << endl;
}
