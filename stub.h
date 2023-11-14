#ifndef STUB_H
#define STUB_H

struct ldops {
    int (*read_headers)(int ifile, uint32_t *r_ent);
    void (*read_sections)(char __far *ptr, int ifile, uint32_t offset);
};

#endif
