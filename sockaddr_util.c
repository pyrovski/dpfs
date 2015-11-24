/*++
  /* DESCRIPTION
  /*These utilities take protocol-independent address structures
  /*and perform protocol-dependent operations on structure members.
  /*Some of the macros described here are called unsafe,
  /*because they evaluate one or more arguments multiple times.
  /*
  /*sock_addr_cmp_addr() or sock_addr_cmp_port() compare the
  /*address family and network address or port fields for
  /*equality, and return indication of the difference between
  /*their arguments:  < 0 if the first argument is "smaller",
  /*0 for equality, and > 0 if the first argument is "larger".
  /*
  /*The unsafe macros SOCK_ADDR_EQ_ADDR() or SOCK_ADDR_EQ_PORT()
  /*compare compare the address family and network address or
  /*port fields for equality, and return non-zero when their
  /*arguments differ.
  /*
  /*sock_addr_in_loopback() determines if the argument specifies
  /*a loopback address.
  /*
  /*The SOCK_ADDR_PTR() macro casts a generic pointer to (struct
  /*sockaddr *).  The name is upper case for consistency not
  /*safety.  SOCK_ADDR_FAMILY() and SOCK_ADDR_LEN() return the
  /*address family and length of the real structure that hides
  /*inside a generic sockaddr structure. On systems where struct
  /*sockaddr has no sa_len member, SOCK_ADDR_LEN() cannot be
  /*used as lvalue. SOCK_ADDR_PORT() returns the IPv4 or IPv6
  /*port number, in network byte order; it must not be used as
  /*lvalue. SOCK_ADDR_PORTP() returns a pointer to the same.
  /*
  /*The macros SOCK_ADDR_IN{,6}_{PTR,FAMILY,PORT,ADDR}() cast
  /*a generic pointer to a specific socket address structure
  /*pointer, or access a specific socket address structure
  /*member. These can be used as lvalues.
  /*
  /*The unsafe INADDR() and IN6_ADDR() macros dereference a
  /*generic pointer to a specific address structure.
  /* DIAGNOSTICS
  /*Panic: unsupported address family.
  /* LICENSE
  /* .ad
  /* .fi
  /*The Secure Mailer license must be distributed with this software.
  /* AUTHOR(S)
  /*Wietse Venema
  /*IBM T.J. Watson Research
  /*P.O. Box 704
  /*Yorktown Heights, NY 10598, USA
  /*--*/

/* System library. */

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

/* Utility library. */

#include "sockaddr_util.h"

/* sock_addr_cmp_addr - compare addresses for equality */

int     sock_addr_cmp_addr(const struct sockaddr * sa,
			   const struct sockaddr * sb)
{
  if (sa->sa_family != sb->sa_family)
    return (sa->sa_family - sb->sa_family);

  /*
   * With IPv6 address structures, assume a non-hostile implementation that
   * stores the address as a contiguous sequence of bits. Any holes in the
   * sequence would invalidate the use of memcmp().
   */
  if (sa->sa_family == AF_INET) {
    return (SOCK_ADDR_IN_ADDR(sa).s_addr - SOCK_ADDR_IN_ADDR(sb).s_addr);
  } else if (sa->sa_family == AF_INET6) {
    return (memcmp((char *) &(SOCK_ADDR_IN6_ADDR(sa)),
		   (char *) &(SOCK_ADDR_IN6_ADDR(sb)),
		   sizeof(SOCK_ADDR_IN6_ADDR(sa))));
  } else {
    /* msg_panic("sock_addr_cmp_addr: unsupported address family %d", */
    /* 	      sa->sa_family); */
  }
}

/* sock_addr_cmp_port - compare ports for equality */

int     sock_addr_cmp_port(const struct sockaddr * sa,
			   const struct sockaddr * sb)
{
  if (sa->sa_family != sb->sa_family)
    return (sa->sa_family - sb->sa_family);

  if (sa->sa_family == AF_INET) {
    return (SOCK_ADDR_IN_PORT(sa) - SOCK_ADDR_IN_PORT(sb));
  } else if (sa->sa_family == AF_INET6) {
    return (SOCK_ADDR_IN6_PORT(sa) - SOCK_ADDR_IN6_PORT(sb));
  } else {
    /* msg_panic("sock_addr_cmp_port: unsupported address family %d", */
    /* 	      sa->sa_family); */
  }
}

/* sock_addr_in_loopback - determine if address is loopback */

int sock_addr_in_loopback(const struct sockaddr * sa)
{
  unsigned long inaddr;

  if (sa->sa_family == AF_INET) {
    inaddr = ntohl(SOCK_ADDR_IN_ADDR(sa).s_addr);
    return (IN_CLASSA(inaddr)
	    && ((inaddr & IN_CLASSA_NET) >> IN_CLASSA_NSHIFT)
	    == IN_LOOPBACKNET);
  } else if (sa->sa_family == AF_INET6) {
    return (IN6_IS_ADDR_LOOPBACK(&SOCK_ADDR_IN6_ADDR(sa)));
  } else {
    /* msg_panic("sock_addr_in_loopback: unsupported address family %d", */
    /* 	      sa->sa_family); */
  }
}
