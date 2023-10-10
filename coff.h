#ifndef COFF_H
#define COFF_H

int read_coff_headers(int ifile, uint32_t *r_ent);
void read_coff_sections(char __far *ptr, int ifile, uint32_t offset);

#endif
