#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "readmap.h"

int main(int argc, char **argv) {
    char outbuf[128], outbuf2[128], outfmt[128], ucname[64];;
    char *outbufp, *outbufp2;
    char *outlines[MAPSIZE], *outlines2[MAPSIZE];
    int len, read, written, longest, outcnt2;

    len = written = longest = outcnt2 = 0;
    
    read = readmap(stdin);

    memset(&outbuf, 0, 128);
    memset(&outbuf2, 0, 128);
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
    printf("mapping _%s[%d] = {\n", argv[1], read);

    outbufp2 = outbuf2;
    outbufp2 += sprintf(outbufp2, "    ");
    
    for (int i = 0; i <= MAPSIZE; i++) {
	int len;

	if (map[i].scancode) {
	    outbufp = outbuf;
	    outbufp += sprintf(outbufp, "    { 0x%02x, 0x%02x",
			       map[i].scancode,
			       map[i].usage_id);
	
	    if (map[i].name)
		outbufp += sprintf(outbufp, ", \"%s\"", map[i].name);
	    if (map[i].labels)
		outbufp += sprintf(outbufp, ", \"%s\"", map[i].labels);

	    if (written == read - 1)
		outbufp += sprintf(outbufp, " }");
	    else
		outbufp += sprintf(outbufp, " },");

	    if (outbuf - outbufp > 80) {
		fprintf(stderr, "line too long\n");
		continue;
	    } else {
		outlines[written] = strdup(outbuf);
		sprintf(outbuf, "&_%s[%d], ", argv[1], written);
		outbufp2 += sprintf(outbufp2, "%-16s", outbuf);
	    }

	    len = strlen(outbuf);

	    if (len > longest)
		longest = len;
	    
	    written++;
	} else {
	    outbufp2 += sprintf(outbufp2, "0,              ");
	}

	if (i % 4 == 3) {
	    outlines2[outcnt2] = strdup(outbuf2);
	    outcnt2++;
	    outbufp2 = outbuf2;
	    outbufp2 += sprintf(outbuf2, "    ");
	}
    }

    if (outbufp2 != outbuf2 + 4) {
	outlines2[outcnt2] = strdup(outbuf2);
	outcnt2++;
    }

    sprintf(outfmt, "%%-%ds /* i = %%-3d */\n", longest + 2);
    
    for (int i = 0; i < written; i++) 
	printf(outfmt, outlines[i], i);

    printf("}\n\n");
    
    printf("mapping *%s[%d] = {\n", argv[1], MAPSIZE);
    
    for (int i = 0; i < outcnt2; i++) 
	printf("%s /* 0x%02x */\n", outlines2[i], (i*4));
    
    printf("}\n\n");
    
    printf("#endif /* _%s_H */\n", ucname);
    
    return 0;
}
