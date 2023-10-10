#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <dos.h>
#include "util.h"

static void farmemcpy(char __far *ptr, unsigned long offset, char *src,
               unsigned long length)
{
    unsigned dummy;
    assert(!(length & 1)); // speed up memcpy
    asm volatile(
          ".arch i386\n"
          "push %%es\n"
          "mov %%dx, %%es\n"
          "mov %[size], %%ecx\n"
          "movzwl %%di, %%edi\n"
          "add %[offs], %%edi\n"
          "shr $1, %%ecx\n"
          "addr32 rep movsw\n"
          "pop %%es\n"
          "xor %%ecx, %%ecx\n"
          "xor %%edi, %%edi\n"
          ".arch i286\n"
        : "=c"(dummy), "=D"(dummy)
        : "a"(0), "d"(FP_SEG(ptr)), "D"(FP_OFF(ptr)), "S"(src),
          [size]"m"(length), [offs]"m"(offset)
        : "memory");
}

void farmemset(char __far *ptr, uint32_t vaddr, uint16_t val, uint32_t size)
{
    unsigned dummy;
    assert(!(size & 1)); // speed up memcpy
    asm volatile(
          ".arch i386\n"
          "push %%es\n"
          "mov %%dx, %%es\n"
          "mov %[size], %%ecx\n"
          "movzwl %%di, %%edi\n"
          "add %[offs], %%edi\n"
          "shr $1, %%ecx\n"
          "addr32 rep stosw\n"
          "pop %%es\n"
          "xor %%ecx, %%ecx\n"
          "xor %%edi, %%edi\n"
          ".arch i286\n"
        : "=c"(dummy), "=D"(dummy)
        : "a"(val), "d"(FP_SEG(ptr)), "D"(FP_OFF(ptr)),
          [size]"m"(size), [offs]"m"(vaddr)
        : "memory");
}

long _long_read(int file, char __far *buf, unsigned long offset,
    unsigned long size)
{
    char tmp[PAGE_SIZE];
    unsigned long done = 0;

#define min(a, b) ((a) < (b) ? (a) : (b))
    while (size) {
        unsigned todo = min(size, sizeof(tmp));
        size_t rd = read(file, tmp, todo);
        if (rd) {
            farmemcpy(buf, offset + done, tmp, rd);
            done += rd;
            size -= rd;
        }
        if (rd < todo)
            break;
    }
    return done;
}
