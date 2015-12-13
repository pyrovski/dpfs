#include "Reader.h"

#include "log.h"

void genericReaderCB(struct bufferevent *bev, void *arg){
  Reader * connection = (Reader *) arg;

  /*!@todo segfault in monitor. I believe the member functions are not
     mapped the way I imagined. Perhaps I am misunderstanding pure
     virtual inheritance?
   */
  dbgmsg("conn: %p, state: %d",
	 connection, connection->getState());
  
  while(connection->enoughBytes())
    connection->processInput();
}
