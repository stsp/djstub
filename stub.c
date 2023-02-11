/*
 *  go32-compatible COFF stub.
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

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#ifndef PAGE_MASK
#define PAGE_MASK	(~(PAGE_SIZE-1))
#endif
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

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

struct coff_header {
    unsigned short 	f_magic;	/* Magic number */
    unsigned short 	f_nscns;	/* Number of Sections */
    int32_t 		f_timdat;	/* Time & date stamp */
    int32_t 		f_symptr;	/* File pointer to Symbol Table */
    int32_t 		f_nsyms;	/* Number of Symbols */
    unsigned short 	f_opthdr;	/* sizeof(Optional Header) */
    unsigned short 	f_flags;	/* Flags */
};

struct opt_header {
    unsigned short 	magic;          /* Magic Number                    */
    unsigned short 	vstamp;         /* Version stamp                   */
    uint32_t 		tsize;          /* Text size in bytes              */
    uint32_t 		dsize;          /* Initialised data size           */
    uint32_t 		bsize;          /* Uninitialised data size         */
    uint32_t 		entry;          /* Entry point                     */
    uint32_t 		text_start;     /* Base of Text used for this file */
    uint32_t 		data_start;     /* Base of Data used for this file */
};

struct scn_header {
    char		s_name[8];	/* Section Name */
    int32_t		s_paddr;	/* Physical Address */
    int32_t		s_vaddr;	/* Virtual Address */
    int32_t		s_size;		/* Section Size in Bytes */
    int32_t		s_scnptr;	/* File offset to the Section data */
    int32_t		s_relptr;	/* File offset to the Relocation table for this Section */
    int32_t		s_lnnoptr;	/* File offset to the Line Number table for this Section */
    unsigned short	s_nreloc;	/* Number of Relocation table entries */
    unsigned short	s_nlnno;	/* Number of Line Number table entries */
    int32_t		s_flags;	/* Flags for this section */
};

enum { SCT_TEXT, SCT_DATA, SCT_BSS, SCT_MAX };

static struct scn_header scns[SCT_MAX];
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

static void farmemcpy(char __far *ptr, unsigned long offset, char *src,
    unsigned long length)
{
    char __far *p = client_memory;
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
        : "a"(0), "d"(FP_SEG(p)), "D"(FP_OFF(p)), "S"(src),
          [size]"m"(length), [offs]"m"(offset)
        : "memory");
}

static long _long_read(int file, char __far *buf, unsigned long offset,
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

static void read_section(int ifile, long coffset, int sc)
{
    long bytes;
    lseek(ifile, coffset + scns[sc].s_scnptr, SEEK_SET);
    bytes = _long_read(ifile, client_memory, scns[sc].s_vaddr,
            scns[sc].s_size);
    stub_debug("read returned %li\n", bytes);
    if (bytes != scns[sc].s_size) {
        fprintf(stderr, "err reading %li bytes, got %li\n",
                scns[sc].s_size, bytes);
        _exit(EXIT_FAILURE);
    }
}

static void farmemset_bss(void)
{
    char __far *p = client_memory;
    uint32_t size = scns[SCT_BSS].s_size;
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
        : "a"(0), "d"(FP_SEG(p)), "D"(FP_OFF(p)),
          [size]"m"(size), [offs]"m"(scns[SCT_BSS].s_vaddr)
        : "memory");
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

int main(int argc, char *argv[], char *envp[])
{
    int ifile;
    off_t coffset = 0;
    uint32_t noffset = 0;
    int rc, i;
#define BUF_SIZE 0x40
    char buf[BUF_SIZE];
    struct coff_header chdr;
    struct opt_header ohdr;
    int done = 0;
    unsigned short clnt_ds;
    unsigned short stubinfo_fs;
    unsigned short mem_hi, mem_lo, si, di;
    unsigned long alloc_size;
    unsigned long stubinfo_mem;
    dpmi_dos_block db;
    char *argv0 = strdup(argv[0]);

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

        stub_debug("Expecting header at %lx\n", coffset);
        rc = read(ifile, buf, BUF_SIZE);
        if (rc != BUF_SIZE) {
            perror("fread()");
            exit(EXIT_FAILURE);
        }
        if (buf[0] == 'M' && buf[1] == 'Z' && buf[8] == 4 /* lfanew */) {
            uint32_t offs;
            cnt++;
            stub_debug("Found exe header %i at %lx\n", cnt, coffset);
            memcpy(&offs, &buf[0x3c], sizeof(offs));
            coffset = offs;
            memcpy(&noffset, &buf[0x1c], sizeof(noffset));
            if (noffset)
                noffset += offs;
        } else if (buf[0] == 0x4c && buf[1] == 0x01) { /* it's a COFF */
            done = 1;
        } else {
            fprintf(stderr, "not an exe %s at %lx\n", argv[0], coffset);
            exit(EXIT_FAILURE);
        }
        lseek(ifile, coffset, SEEK_SET);
    }

    rc = read(ifile, &chdr, sizeof(chdr)); /* get the COFF header */
    if (rc != sizeof(chdr) || chdr.f_opthdr != sizeof(ohdr)) {
        fprintf(stderr, "bad COFF header\n");
        exit(EXIT_FAILURE);
    }
    rc = read(ifile, &ohdr, sizeof(ohdr)); /* get the COFF opt header */
    if (rc != sizeof(ohdr)) {
        fprintf(stderr, "bad COFF opt header\n");
        exit(EXIT_FAILURE);
    }
    rc = read(ifile, scns, sizeof(scns[0]) * SCT_MAX);
    if (rc != sizeof(scns[0]) * SCT_MAX) {
        fprintf(stderr, "failed to read section headers\n");
        exit(EXIT_FAILURE);
    }
#if STUB_DEBUG
    for (i = 0; i < SCT_MAX; i++) {
        struct scn_header *h = &scns[i];
        stub_debug("Section %s pa %lx va %lx size %lx foffs %lx\n",
                h->s_name, h->s_paddr, h->s_vaddr, h->s_size, h->s_scnptr);
    }
#endif
    strncpy(stubinfo.magic, "go32stub,v3,stsp", sizeof(stubinfo.magic));
    stubinfo.size = sizeof(stubinfo);
    i = 0;
    while(*envp) {
        i += strlen(*envp) + 1;
        envp++;
    }
    if (i) {
        i += strlen(argv0) + 1;
        i += 2;
    }
    stub_debug("env size %i\n", i);
    stubinfo.env_size = i;
    stubinfo.minstack = 0x80000;
    stubinfo.minkeep = 0x4000;
    strncpy(stubinfo.argv0, _basename(argv0), sizeof(stubinfo.argv0));
    strncpy(stubinfo.basename, _fname(argv0), sizeof(stubinfo.basename));
    strncpy(stubinfo.dpmi_server, "CWSDPMI.EXE", sizeof(stubinfo.dpmi_server));
#define max(a, b) ((a) > (b) ? (a) : (b))
    stubinfo.initial_size = max(scns[SCT_BSS].s_vaddr + scns[SCT_BSS].s_size,
        0x10000);
    stubinfo.psp_selector = psp_sel;
    /* DJGPP relies on ds_selector, cs_selector and ds_segment all mapping
     * the same real-mode memory block. */
    link_umb(1);
    db = _DPMIAllocateDOSMemoryBlock(stubinfo.minkeep >> 4);
    link_umb(0);
    stub_debug("rm seg %x\n", db.rm);
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
    clnt_entry.offset32 = ohdr.entry;
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
    client_memory = MK_FP(clnt_ds, 0);
    /* set base */
    asm volatile("int $0x31\n"
        :
        : "a"(7), "b"(clnt_entry.selector), "c"(mem_hi), "d"(mem_lo)
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
          "c"((uint16_t)(stubinfo.initial_size >> 16)),
          "d"(0xffff)
        : "cc");
    /* set base */
    asm volatile("int $0x31\n"
        :
        : "a"(7), "b"(clnt_ds), "c"(mem_hi), "d"(mem_lo)
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
          "c"((uint16_t)(stubinfo.initial_size >> 16)),
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

    read_section(ifile, coffset, SCT_TEXT);
    read_section(ifile, coffset, SCT_DATA);
    farmemset_bss();

    stubinfo.self_fd = ifile;
    stubinfo.payload_offs = noffset;
    stubinfo.payload_size = noffset ? lseek(ifile, 0, SEEK_END) - noffset : 0;
    lseek(ifile, noffset, SEEK_SET);
    if (stubinfo.payload_size > 0) {
        stub_debug("Found payload of size %li at %lx\n",
                stubinfo.payload_size, stubinfo.payload_offs);
    }
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
