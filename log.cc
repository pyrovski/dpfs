#include <iostream>
#include <chrono>

#include <stdlib.h>
#include <math.h>

#include "log.hh"

using namespace std;

log_t::log_t(const char * file){
  log_fstream.open(file, ios_base::app);
  cout << file << ": " << log_fstream.is_open() << endl;

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
  log_fstream <<
    duration.count() * chrono::high_resolution_clock::period::num / chrono::high_resolution_clock::period::den <<
    "." <<
    //!@todo fix format; want 0-padded most-significant n digits
    fmodf(duration.count() * chrono::high_resolution_clock::period::num,
	  chrono::high_resolution_clock::period::den) <<
    ": "  << 
    str <<
    endl;
}
