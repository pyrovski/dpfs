#include "Reader.h"

void genericReaderCB(struct bufferevent *bev, void *arg){
  Reader * connection = (Reader *) arg;
  const log_t &log = connection->getLog();

  /*!@todo segfault in monitor. I believe the member functions are not
     mapped the way I imagined. Perhaps I am misunderstanding pure
     virtual inheritance?
   */
  dbgmsg(log, "conn: %p, state: %d",
	 connection, connection->getState());
  
  while(connection->enoughBytes())
    connection->processInput();
}
