/*
 *  Copyright (C) 2023-2026  stsp2@yandex.ru
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "dpmi.h"
#include "util.h"

#define STUB_DEBUG 0
#if STUB_DEBUG
#define stub_debug(...) printf(__VA_ARGS__)
#else
#define stub_debug(...)
#endif

unsigned short psp_sel;

static void farmemcpy(char_far ptr, unsigned long offset, char *src,
               unsigned long length)
{
    unsigned dummy;
    assert(!(length & 1)); // speed up memcpy
#ifdef __GNUC__
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
        : "a"(0), "d"(__FP_SEG(ptr)), "D"(__FP_OFF(ptr)), "S"(src),
          [size]"m"(length), [offs]"m"(offset)
        : "memory");
#else
    uint32_t base;
    int rc = dpmi_get_segment_base_address(__FP_SEG(ptr), &base);
    assert(rc != -1);
    base += __FP_OFF(ptr);
    memcpy((char *)base + offset, src, length);
#endif
}

void farmemset(char_far ptr, uint32_t offset, uint8_t val, uint32_t size)
{
    unsigned dummy;
    assert(!(size & 1)); // speed up memcpy
#ifdef __GNUC__
    asm volatile(
          ".arch i386\n"
          "push %%es\n"
          "mov %%dx, %%es\n"
          "mov %[size], %%ecx\n"
          "movzwl %%di, %%edi\n"
          "add %[offs], %%edi\n"
          "shr $1, %%ecx\n"
          "mov %%al, %%ah\n"
          "addr32 rep stosw\n"
          "pop %%es\n"
          "xor %%ecx, %%ecx\n"
          "xor %%edi, %%edi\n"
          ".arch i286\n"
        : "=c"(dummy), "=D"(dummy)
        : "a"(val), "d"(__FP_SEG(ptr)), "D"(__FP_OFF(ptr)),
          [size]"m"(size), [offs]"m"(offset)
        : "memory");
#else
    uint32_t base;
    int rc = dpmi_get_segment_base_address(__FP_SEG(ptr), &base);
    assert(rc != -1);
    base += __FP_OFF(ptr);
    memset((char *)base + offset, val, size);
#endif
}

long _long_read(int file, char_far buf, unsigned long offset,
    unsigned long size)
{
    char tmp[PAGE_SIZE];
    unsigned long done = 0;

#define min(a, b) ((a) < (b) ? (a) : (b))
    while (size) {
        unsigned todo = min(size, sizeof(tmp));
        size_t rd = read(file, tmp, todo);
        if (rd) {
            /* word-align memcpy */
            farmemcpy(buf, offset + done, tmp, rd + (rd & 1));
            done += rd;
            size -= rd;
        }
        if (rd < todo)
            break;
    }
    return done;
}

#ifndef __GNUC__
static unsigned lcall(unsigned sel, unsigned off, unsigned ax)
{
  asm(
    "push es\n"
    "push dword [ebp+8]\n "  // sel
    "push dword [ebp+12]\n"  // off
    "mov eax, [ebp+16]\n"    // ax
    "call far [ss:esp]\n"
    "pop eax\n"
    "pop eax\n"
    "pushf\n"
    "pop eax\n"
    "mov [_psp_sel], es\n"
    "pop es\n"
  );
}

static void int2f(__dpmi_int_regs *r)
{
  asm(
    "push es\n"
    "mov ebx, [ebp+8]\n"
    "mov edi, [ebx]\n"
    "mov esi, [ebx+4]\n"
    "mov edx, [ebx+20]\n"
    "mov ecx, [ebx+24]\n"
    "mov eax, [ebx+28]\n"
    "mov ebx, [ebx+16]\n"
    "stc\n"
    "int 0x2f\n"
    "push ebx\n"
    "mov ebx, [ebp+8]\n"
    "pop dword [ebx+16]\n"
    "pushfw\n"
    "pop word [ebx+32]\n"
    "mov [ebx], edi\n"
    "mov [ebx+4], esi\n"
    "mov [ebx+20], edx\n"
    "mov [ebx+24], ecx\n"
    "mov [ebx+28], eax\n"
    "mov [ebx+34], es\n"
    "mov [ebx+36], ds\n"
    "mov [ebx+38], fs\n"
    "mov [ebx+40], gs\n"
    "pop es\n"
  );
}
#endif

int dpmi_init(void)
{
#ifdef __GNUC__
    union REGPACK r = {};
    char_far sw;
    unsigned mseg = 0, f;

#define CF 1
    r.w.ax = 0x1687;
    intr(0x2f, &r);
    if ((r.w.flags & CF) || r.w.ax != 0) {
        stub_debug("DPMI unavailable\n");
        return -1;
    }
    if (!(r.w.bx & 1)) {
        fprintf(stderr, "DPMI-32 unavailable\n");
        exit(EXIT_FAILURE);
    }
    sw = __MK_FP(r.w.es, r.w.di);
    if (r.w.si) {
#if 0
        link_umb(1);
        err = _dos_allocmem(r.w.si, &mseg);
        link_umb(0);
        if (err) {
            fprintf(stderr, "malloc of %i para failed\n", r.w.si);
            exit(EXIT_FAILURE);
        }
#else
        fprintf(stderr, "Invalid DPMI server\n");
        exit(EXIT_FAILURE);
#endif
    }
    asm volatile(
        "mov %[mseg], %%es\n"
        "lcall *%[sw]\n"
        "pushf\n"
        "pop %0\n"
        "mov %%es, %1\n"
        "push %%ds\n"
        "pop %%es\n"
        : "=r"(f), "=r"(psp_sel)
        : "a"(0x501), [mseg]"r"(mseg), [sw]"m"(sw)
        : "cc", "memory");
    if (f & CF) {
        fprintf(stderr, "DPMI init failed\n");
        exit(EXIT_FAILURE);
    }
#else
    __dpmi_int_regs r = {0};
    unsigned f;

#define CF 1
    r.eax = 0x1687;
    int2f(&r);
    if ((r.flags & CF) || r.eax != 0) {
        stub_debug("DPMI unavailable\n");
        return -1;
    }
    if (!(r.ebx & 1)) {
        fprintf(stderr, "DPMI-32 unavailable\n");
        exit(EXIT_FAILURE);
    }
    if (r.esi) {
        fprintf(stderr, "invalid DPMI server, %i para requested\n", r.esi);
        exit(EXIT_FAILURE);
    }
    f = lcall(r.es, r.edi, 1);
    if (f & CF) {
        fprintf(stderr, "DPMI init failed\n");
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
