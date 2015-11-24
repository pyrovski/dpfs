#ifndef MONITORCONTEXT_H
#define MONITORCONTEXT_H

#include "event.h"

class monitor;

class monitorContext {
 public:
 monitorContext():
  base(NULL), mon(NULL)
  {
  }
  struct event_base * base;
  monitor * mon;
};

#endif
