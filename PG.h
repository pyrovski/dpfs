#ifndef PG_H
#define PG_H

/*!@todo constructed from PG map on monitor. Under the hood, each PG
   has its own method of storing data across OSDs. This is necessary
   to smoothly transition between data layouts.

   From an outside perspective, the PG just handles data access
   requests. Clients should request a path from the MDS, get an object
   number and corresponding piece number, and calculate their
   hash. The hash should map to a PG. Without including the piece
   number, the size of a PG would limit the size of an object.
 */

#endif
