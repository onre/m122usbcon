#ifndef _READMAP_H
#define _READMAP_H

#include "mapping.h"

#define MAPSIZE 0xff
#define BUFSIZE 80

int readmap(FILE *stream);
extern mapping map[MAPSIZE];

#endif /* _READMAP_H */
