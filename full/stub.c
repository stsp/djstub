/*
 *  go32-compatible COFF, PE32 and ELF loader stub.
 *  Copyright (C) 2022,  stsp <stsp@users.sourceforge.net>
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <dos.h>
#include <dpmi.h>
#include <assert.h>
#include "stubinfo.h"
#include "coff.h"
#include "elfp.h"
#include "util.h"
#include "stub.h"

#define STUB_DEBUG 0
#if STUB_DEBUG
#define stub_debug(...) printf(__VA_ARGS__)
#else
#define stub_debug(...)
#endif

typedef struct
{
    uint32_t offset32;
    unsigned short selector;
} DPMI_FP;

static unsigned short psp_sel;
static char __far *client_memory;
static DPMI_FP clnt_entry;
static _GO32_StubInfo stubinfo;

static void link_umb(int on)
{
    asm volatile("int $0x21\n"
        :
        : "a"(0x5803), "b"(on)
        : "cc");
    if (on) {
        asm volatile("int $0x21\n"
            :
            : "a"(0x5801), "b"(0x80)
            : "cc");
    }
}

static void dpmi_init(void)
{
    union REGPACK r = {};
    void __far (*sw)(void);
    unsigned mseg = 0, f;
    int err;

#define CF 1
    r.w.ax = 0x1687;
    intr(0x2f, &r);
    if ((r.w.flags & CF) || r.w.ax != 0) {
        fprintf(stderr, "DPMI unavailable\n");
        exit(EXIT_FAILURE);
    }
    if (!(r.w.bx & 1)) {
        fprintf(stderr, "DPMI-32 unavailable\n");
        exit(EXIT_FAILURE);
    }
    sw = MK_FP(r.w.es, r.w.di);
    if (r.w.si) {
        link_umb(1);
        err = _dos_allocmem(r.w.si, &mseg);
        link_umb(0);
        if (err) {
            fprintf(stderr, "malloc of %i para failed\n", r.w.si);
            exit(EXIT_FAILURE);
        }
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
        : "a"(1), [mseg]"r"(mseg), [sw]"m"(sw)
        : "cc", "memory");
    if (f & CF) {
        fprintf(stderr, "DPMI init failed\n");
        exit(EXIT_FAILURE);
    }
}

static const char *_basename(const char *name)
{
    const char *p;
    p = strrchr(name, '\\');
    if (!p)
        p = name;
    else
        p++;
    return p;
}

#if 0
static char *_fname(char *name)
{
    char *p, *p1;
    p = strrchr(name, '\\');
    if (!p)
        p = name;
    else
        p++;
    p1 = strrchr(p, '.');
    if (p1)
        p1[0] = '\0';
    return p;
}
#endif

int main(int argc, char *argv[], char *envp[])
{
    int ifile;
    off_t coffset = 0;
    uint32_t coffsize = 0;
    uint32_t noffset = 0;
    uint32_t nsize = 0;
    uint32_t noffset2 = 0;
    uint32_t nsize2 = 0;
    int rc, i;
#define BUF_SIZE 0x40
    char buf[BUF_SIZE];
    int done = 0;
    uint32_t va;
    uint32_t va_size;
    uint32_t mem_lin;
    uint32_t mem_base;
    unsigned short clnt_ds;
    unsigned short stubinfo_fs;
    unsigned short mem_hi, mem_lo, si, di;
    unsigned long alloc_size;
    unsigned long stubinfo_mem;
    dpmi_dos_block db;
    void *handle;
    struct ldops *ops = NULL;

    if (argc == 0) {
        fprintf(stderr, "no env\n");
        exit(EXIT_FAILURE);
    }
    dpmi_init();

    ifile = open(argv[0], O_RDONLY);
    if (ifile == -1) {
        fprintf(stderr, "cannot open %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    while (!done) {
        int cnt = 0;

        stub_debug("Expecting header at 0x%lx\n", coffset);
        rc = read(ifile, buf, BUF_SIZE);
        if (rc != BUF_SIZE) {
            perror("fread()");
            exit(EXIT_FAILURE);
        }
        if (buf[0] == 'M' && buf[1] == 'Z' && buf[8] == 4 /* lfanew */) {
            uint32_t offs;
            cnt++;
            stub_debug("Found exe header %i at 0x%lx\n", cnt, coffset);
            memcpy(&offs, &buf[0x3c], sizeof(offs));
            coffset = offs;
            memcpy(&coffsize, &buf[0x1c], sizeof(coffsize));
            if (coffsize)
                noffset = coffset + coffsize;
            memcpy(&nsize, &buf[0x20], sizeof(nsize));
            if (nsize)
                noffset2 = noffset + nsize;
            memcpy(&nsize2, &buf[0x24], sizeof(nsize2));
            strncpy(stubinfo.payload2_name, &buf[0x30], 12);
            stubinfo.payload2_name[12] = '\0';
        } else if (buf[0] == 0x4c && buf[1] == 0x01) { /* it's a COFF */
            done = 1;
            ops = &coff_ops;
        } else if (buf[0] == 0x7f && buf[1] == 0x45 &&
                buf[2] == 0x4c && buf[3] == 0x46) { /* it's an ELF */
            done = 1;
            ops = &elf_ops;
        } else {
            fprintf(stderr, "not an exe %s at %lx\n", argv[0], coffset);
            exit(EXIT_FAILURE);
        }
        lseek(ifile, coffset, SEEK_SET);
    }

    assert(ops);
    handle = ops->read_headers(ifile);
    if (!handle)
        exit(EXIT_FAILURE);
    va = ops->get_va(handle);
    va_size = ops->get_length(handle);
    clnt_entry.offset32 = ops->get_entry(handle);
    stub_debug("va 0x%lx va_size 0x%lx entry 0x%lx\n",
            va, va_size, clnt_entry.offset32);

    strncpy(stubinfo.magic, "go32stub,v3,stsp", sizeof(stubinfo.magic));
    stubinfo.size = sizeof(stubinfo);
    i = 0;
    while(*envp) {
        i += strlen(*envp) + 1;
        envp++;
    }
    if (i) {
        i += strlen(argv[0]) + 1;
        i += 3;
    }
    stub_debug("env size %i\n", i);
    stubinfo.env_size = i;
    stubinfo.minstack = 0x80000;
    stubinfo.minkeep = 0x4000;
    strncpy(stubinfo.argv0, _basename(argv[0]), sizeof(stubinfo.argv0));
    stubinfo.argv0[sizeof(stubinfo.argv0) - 1] = '\0';
//    strncpy(stubinfo.basename, _fname(argv0), sizeof(stubinfo.basename));
//    stubinfo.basename[sizeof(stubinfo.basename) - 1] = '\0';
    strncpy(stubinfo.dpmi_server, "CWSDPMI.EXE", sizeof(stubinfo.dpmi_server));
#define max(a, b) ((a) > (b) ? (a) : (b))
    stubinfo.initial_size = max(va_size, 0x10000);
    stubinfo.psp_selector = psp_sel;
    /* DJGPP relies on ds_selector, cs_selector and ds_segment all mapping
     * the same real-mode memory block. */
    link_umb(1);
    db = _DPMIAllocateDOSMemoryBlock(stubinfo.minkeep >> 4);
    link_umb(0);
    stub_debug("rm seg 0x%x\n", db.rm);
    stubinfo.ds_selector = db.pm;
    stubinfo.ds_segment = db.rm;
    /* create alias */
    asm volatile("int $0x31\n"
        : "=a"(stubinfo.cs_selector)
        : "a"(0xa), "b"(db.pm));
    /* set descriptor access rights */
    asm volatile("int $0x31\n"
        :
        : "a"(9), "b"(stubinfo.cs_selector), "c"(0x00fb)
        : "cc");

    clnt_entry.selector = _DPMIAllocateLDTDescriptors(1);
    clnt_ds = _DPMIAllocateLDTDescriptors(1);
    alloc_size = PAGE_ALIGN(stubinfo.initial_size);
    /* allocate mem */
    asm volatile("int $0x31\n"
        : "=b"(mem_hi), "=c"(mem_lo), "=S"(si), "=D"(di)
        : "a"(0x501),
          "b"((uint16_t)(alloc_size >> 16)),
          "c"((uint16_t)(alloc_size & 0xffff))
        : "cc");
    stubinfo.memory_handle = ((uint32_t)si << 16) | di;
    mem_lin = ((uint32_t)mem_hi << 16) | mem_lo;
    mem_base = mem_lin - va;
    stubinfo.mem_base = mem_base;
    stub_debug("mem_lin 0x%lx mem_base 0x%lx\n", mem_lin, mem_base);
    client_memory = MK_FP(clnt_ds, 0);
    /* set base */
    asm volatile("int $0x31\n"
        :
        : "a"(7),
          "b"(clnt_entry.selector),
          "c"((uint16_t)(mem_base >> 16)),
          "d"((uint16_t)(mem_base & 0xffff))
        : "cc");
    /* set descriptor access rights */
    asm volatile("int $0x31\n"
        :
        : "a"(9), "b"(clnt_entry.selector), "c"(0xc0fb)
        : "cc");
    /* set limit */
    asm volatile("int $0x31\n"
        :
        : "a"(8), "b"(clnt_entry.selector),
          "c"(0xffff),
          "d"(0xffff)
        : "cc");
    /* set base */
    asm volatile("int $0x31\n"
        :
        : "a"(7),
          "b"(clnt_ds),
          "c"((uint16_t)(mem_base >> 16)),
          "d"((uint16_t)(mem_base & 0xffff))
        : "cc");
    /* set descriptor access rights */
    asm volatile("int $0x31\n"
        :
        : "a"(9), "b"(clnt_ds), "c"(0xc0f3)
        : "cc");
    /* set limit */
    asm volatile("int $0x31\n"
        :
        : "a"(8), "b"(clnt_ds),
          "c"(0xffff),
          "d"(0xffff)
        : "cc");

    /* create ds alias */
    asm volatile(
        "mov %%ds, %%bx\n"
        "int $0x31\n"
        : "=a"(stubinfo_fs)
        : "a"(0xa));
    stubinfo_mem = _DPMIGetSegmentBaseAddress(stubinfo_fs) +
            (uintptr_t)&stubinfo;
    mem_hi = stubinfo_mem >> 16;
    mem_lo = stubinfo_mem & 0xffff;
    /* set base */
    asm volatile("int $0x31\n"
        :
        : "a"(7), "b"(stubinfo_fs), "c"(mem_hi), "d"(mem_lo)
        : "cc");

    ops->read_sections(handle, client_memory, ifile, coffset);

    stubinfo.self_fd = ifile;
    stubinfo.self_offs = coffset;
    stubinfo.self_size = coffsize;
    stubinfo.payload_offs = noffset;
    stubinfo.payload_size = nsize;
    stubinfo.payload2_offs = noffset2;
    stubinfo.payload2_size = nsize2;
    if (stubinfo.payload2_name[0])
        strcat(stubinfo.payload2_name, ".dbg");
    lseek(ifile, noffset, SEEK_SET);
    if (nsize > 0)
        stub_debug("Found payload of size %li at 0x%lx\n", nsize, noffset);
    stubinfo.stubinfo_ver = 2;

    stub_debug("Jump to entry...\n");
    asm volatile(
          ".arch i386\n"
          "mov %%ax, %%fs\n"
          "push %%ds\n"
          "pop %%es\n"
          "mov %1, %%ds\n"
          "ljmpl *%%es:%2\n"
          ".arch i286\n"
        :
        : "a"(stubinfo_fs), "r"(clnt_ds), "m"(clnt_entry)
        : "memory");
    return 0;
}
