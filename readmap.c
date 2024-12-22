#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include "mapping.h"
#include "readmap.h"

mapping map[MAPSIZE];

int readmap(FILE *stream) {
    mapping t;
    char *line, *p;
    int len, read, lineno;

    read = lineno = 0;

    line = malloc(BUFSIZE);

    if (!line)
	return -1;

    memset(map, 0, sizeof map);
    memset(line, 0, BUFSIZE);

    while (fgets(line, BUFSIZE - 1, stream)) {
	lineno++;
	if (!isxdigit(line[0]))
	    continue;
	
	p = strtok(line, ":");
	if (p)
	    t.scancode = 0xff & strtoumax(p, NULL, 16);

	if (!t.scancode) {
	    fprintf(stderr,
		    "readmap: input line %d: can't parse scancode\n",
		    lineno);
	    continue;
        }

        p = strtok(NULL, ":");
	if (p)
	    t.usage_id = 0xff & strtoumax(p, NULL, 16);
        else {
          fprintf(stderr, "readmap: input line %d: can't parse usage id\n",
                  lineno);
          continue;
        }

        p = strtok(NULL, ":");
        if (p && p[0] != ':' && p[0] != '\n')
	    t.name = strdup(p);
	else
	    t.name = 0;

        p = strtok(NULL, ":");
        if (p && p[0] != ':' && p[0] != '\n')
	    t.labels = strndup(p, strlen(p) - 1); /* skip \n */
	else
	    t.labels = 0;
	
        memcpy(&map[t.scancode], &t, sizeof(mapping));
	read++;
    }

    fprintf(stderr, "readmap: %d lines read\n", lineno);
    
    return read;
}
