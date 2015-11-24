#ifndef TIME_H
#define TIME_H

#include <sys/time.h>
#include "pbTime.pb.h"

inline void getTime(pbTime::Time &tv_pb){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  tv_pb.set_seconds(tv.tv_sec);
  tv_pb.set_microseconds(tv.tv_usec);
}
#endif
