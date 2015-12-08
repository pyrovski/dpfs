#ifndef READER_H
#define READER_H

#include "log.h"

class Reader {
 public:
  virtual const log_t & getLog() const = 0;
  virtual bool enoughBytes() const = 0;
  virtual void processInput() = 0;
  virtual int getState() const = 0;
};


void genericReaderCB(struct bufferevent *bev, void *arg);

#endif
