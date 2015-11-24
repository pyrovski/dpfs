#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

inline bool bigEndian(void){
  uint8_t swaptest[2] = {1, 0};
  if ( *(uint16_t *) swaptest == 1)
    return false;
  return true;
}

#endif
