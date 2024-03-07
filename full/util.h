#ifndef UTIL_H
#define UTIL_H

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#ifndef PAGE_MASK
#define PAGE_MASK	(~(PAGE_SIZE-1))
#endif
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

void farmemset(char __far *ptr, uint32_t vaddr, uint16_t val, uint32_t size);
long _long_read(int file, char __far *buf, unsigned long offset,
    unsigned long size);

#endif
