#ifndef TIME_H
#define TIME_H

#include <math.h>
#include <sys/time.h>
#include "pbTime.pb.h"

inline void getTime(pbTime::Time &tv_pb){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  tv_pb.set_seconds(tv.tv_sec);
  tv_pb.set_microseconds(tv.tv_usec);
}

inline void tvFromPB(const pbTime::Time &tv_pb, struct timeval &tv){
  tv.tv_sec = tv_pb.seconds();
  tv.tv_usec = tv_pb.microseconds();
}

inline double tvDiff(const struct timeval &lhs, const struct timeval &rhs){
  double result = lhs.tv_sec - rhs.tv_sec;
  if(lhs.tv_usec < rhs.tv_usec)
    ++result;
  result += (lhs.tv_usec - rhs.tv_usec)/1e6;
  
  return result;
}

inline double to_double(const struct timeval &tv){
  return (double) tv.tv_sec + (double) tv.tv_usec/ 1e6;
}

inline struct timeval to_tv(double val){
  struct timeval tv;
  double intpart;
  tv.tv_sec = val;
  tv.tv_usec = modf(val, &intpart) / 0.000001;
  return tv;
}

#endif
