#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "readmap.h"

int main(int argc, char **argv) {
    char outbuf[128], ucname[64];;
    char *outbufp;
    char *outlines[MAPSIZE];
    int len, read, written, longest;

    len = written = longest = 0;
    
    read = readmap(stdin);

    memset(outbuf, 0, 128);
    memset(ucname, 0, 64);
    
    if (!read) {
	fprintf(stderr, "no key mappings found in input\n");
	return -1;
    }

    if (argc != 2) {
	fprintf(stderr, "usage: mkheader varname\n");
	return -1;
    }

    len = strlen(argv[1]);

    for (int i = 0; i < len; i++) {
	ucname[i] = toupper(argv[1][i]);
    }
    
    printf("#ifndef _%s_H\n#define _%s_H\n\n", ucname, ucname);
    printf("#include \"mapping.h\"\n\n");
    printf("#define KEYCOUNT\t%d\n\n", read);
    printf("mapping %s[%d] = {\n", argv[1], MAPSIZE);

    for (int i = 0; i < MAPSIZE; i++) {
	int len;
	outbufp = outbuf;

	if (map[i].scancode) {
	    outbufp += sprintf(outbufp, "    { 0x%02x, 0x%02x",
			       map[i].scancode,
			       map[i].usage_id);
	
	    if (map[i].name)
		outbufp += sprintf(outbufp, ", \"%s\"", map[i].name);
	    if (map[i].labels)
		outbufp += sprintf(outbufp, ", \"%s\"", map[i].labels);

	    if (i == MAPSIZE - 1)
		outbufp += sprintf(outbufp, " }");
	    else
		outbufp += sprintf(outbufp, " },");

	    outlines[written] = strdup(outbuf);

	    len = strlen(outbuf);

	    if (len > longest)
		longest = len;
	    
	} else {
	    if (i == MAPSIZE - 1)
		outbufp += sprintf(outbufp, "    { 0 }");
	    else
		outbufp += sprintf(outbufp, "    { 0 },");
	    outlines[written] = strdup(outbuf);
	}

	written++;
    }

    for (int i = 0; i < written; i++) 
	printf("%s\n", outlines[i]);

    printf("};\n\n");
    
    printf("#endif /* _%s_H */\n", ucname);
    
    return 0;
}
