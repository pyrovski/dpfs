#include "uuid_s.h"

bool uuid_s::operator == (const uuid_s & other) const {
  return !uuid_compare(uuid, other.uuid);
}
