#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#ifndef PAGE_MASK
#define PAGE_MASK	(~(PAGE_SIZE-1))
#endif
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

typedef struct {
    unsigned short off;
    unsigned short seg;
} char_far;

#ifndef __GNUC__
typedef struct
{
  unsigned long  edi;
  unsigned long  esi;
  unsigned long  ebp;
  unsigned long  resrvd;
  unsigned long  ebx;
  unsigned long  edx;
  unsigned long  ecx;
  unsigned long  eax;
  unsigned short flags;
  unsigned short es;
  unsigned short ds;
  unsigned short fs;
  unsigned short gs;
  unsigned short ip;
  unsigned short cs;
  unsigned short sp;
  unsigned short ss;
} __dpmi_int_regs;
#endif

#define __MK_FP(s, o) (char_far){ .seg = (s), .off = (o) }
#define __FP_SEG(f) (f).seg
#define __FP_OFF(f) (f).off

void farmemset(char_far ptr, uint32_t offset, uint8_t val, uint32_t size);
long _long_read(int file, char_far buf, unsigned long offset,
    unsigned long size);
int dpmi_init(void);

extern unsigned short psp_sel;

#endif
