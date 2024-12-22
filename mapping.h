#ifndef _MAPPING_H
#define _MAPPING_H

typedef struct mapping {
    unsigned char scancode;
    unsigned char usage_id;
    char *name;
    char *labels;
} mapping;

#endif /* _MAPPING_H */
