#ifndef UUID_H
#define UUID_H

#include <string>

#include <uuid/uuid.h>

struct uuid_s {
  uuid_t uuid;
  bool operator == (const uuid_s & other) const;
};

namespace std {
  template <> struct hash<uuid_s> {
    std::size_t operator()(const uuid_s& k) const {
      using std::size_t;
      using std::hash;
      using std::string;
      return
	hash<uint64_t>()(*(uint64_t *)k.uuid) << 1 ^
	hash<uint64_t>()(
			 *(
			   (uint64_t *)k.uuid + 1
			   )
			 );
    }
  };
}

#endif
