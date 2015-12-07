#ifndef READER_H
#define READER_H

class Reader {
 public:
  virtual const log_t & getLog() const = 0;
  virtual bool enoughBytes() const = 0;
  virtual void processInput() = 0;
  virtual int getState() const = 0;
};

#endif
