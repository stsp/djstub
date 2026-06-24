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
#ifdef __GNUC__
#include <dpmi.h>
#else
#include "dpmi.h"
#endif
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
static char_far client_memory;
static DPMI_FP clnt_entry;
static _GO32_StubInfo stubinfo;

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

typedef struct {
    uint16_t rm, pm;
} dpmi_dos_block;
#endif

static void link_umb(int on)
{
#ifdef __GNUC__
    asm volatile("int $0x21\n"
        :
        : "a"(0x5803), "b"(on)
        : "cc");
#else
    asm(
        "mov eax, 0x5803\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x21\n"
    );
#endif
    if (on) {
#ifdef __GNUC__
        asm volatile("int $0x21\n"
            :
            : "a"(0x5801), "b"(0x80)
            : "cc");
#else
        asm(
            "mov eax, 0x5801\n"
            "mov ebx, 0x80\n"
            "int 0x21\n"
        );
#endif
    }
}

#ifndef __GNUC__
extern void* __dpmi_psp;
extern void* __dpmi_env;

static unsigned short alloc_psp(void)
{
  asm(
    "mov eax, 0\n"    // alloc desc for PSP
    "mov ecx, 1\n"    // 1 desc
    "int 31h\n"
    "push ax\n"       // psp
    "mov eax, 7\n"    // set base
    "pop bx\n"
    "push bx\n"
    "extern  ___dpmi_psp\n"
    "mov cx, [___dpmi_psp + 2]\n"
    "mov dx, [___dpmi_psp]\n"
    "int 31h\n"
    "mov eax, 8\n"    // set limit
    "pop bx\n"
    "push bx\n"
    "mov cx, 0\n"
    "mov dx, 0ffh\n"  // to PSP size
    "int 31h\n"
    "pop ax\n"
  );
}

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

static unsigned short __my_ds(void)
{
  asm("mov ax, ds\n");
}

static void jump_to_entry(unsigned stubinfo_fs, unsigned clnt_ds)
{
  asm(
    "mov fs, [ebp + 8]\n"
    "push ds\n"
    "pop es\n"
    "mov ds, [ebp + 12]\n"
    "jmp far dword [es:_clnt_entry]\n"
  );
}
#endif

static void dpmi_init(void)
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
        psp_sel = _psp;
        return;
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
        psp_sel = alloc_psp();
        return;
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
    uint8_t stub_ver = 0;
    const char *fname = argv[0];

    if (argc == 0) {
        fprintf(stderr, "no env\n");
        exit(EXIT_FAILURE);
    }
    dpmi_init();

    for (i = 0; envp && envp[i]; i++) {
        const char *s = "ELFEXEC=";
        int l = strlen(s);

        if (strncmp(envp[i], s, l) == 0) {
            fname = envp[i] + l;
            done = 1;
            ops = &elf_ops;
            break;
        }
    }

    ifile = open(fname, O_RDONLY);
    if (ifile == -1) {
        fprintf(stderr, "cannot open %s\n", fname);
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
        if (buf[0] == 'M' && buf[1] == 'Z' && buf[8] == 4 && buf[9] == 0) {
            /* lfanew */
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
            stub_ver = buf[0x3b];
            stubinfo.stubinfo_ver |= (uint32_t)stub_ver << 16;
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
    i = 3;
    while(envp && *envp) {
        i += strlen(*envp) + 1;
        envp++;
    }
    if (argv && argv[0])
        i += strlen(argv[0]) + 1;
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
#ifdef __GNUC__
    db = _DPMIAllocateDOSMemoryBlock(stubinfo.minkeep >> 4);
#else
    db.rm = dpmi_allocate_dos_memory(stubinfo.minkeep >> 4, &db.pm);
#endif
    link_umb(0);
    stub_debug("rm seg 0x%x\n", db.rm);
    stubinfo.ds_selector = db.pm;
    stubinfo.ds_segment = db.rm;
    /* create alias */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        : "=a"(stubinfo.cs_selector)
        : "a"(0xa), "b"(db.pm));
#else
    stubinfo.cs_selector = dpmi_create_alias_descriptor(db.pm);
#endif
    /* set descriptor access rights */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(9), "b"(stubinfo.cs_selector), "c"(0x00fb)
        : "cc");
#else
    dpmi_set_descriptor_access_rights(stubinfo.cs_selector, 0xfb);
#endif

#ifdef __GNUC__
    clnt_entry.selector = _DPMIAllocateLDTDescriptors(1);
    clnt_ds = _DPMIAllocateLDTDescriptors(1);
#else
    clnt_entry.selector = dpmi_allocate_ldt_descriptors(1);
    clnt_ds = dpmi_allocate_ldt_descriptors(1);
#endif
    alloc_size = PAGE_ALIGN(stubinfo.initial_size);
    /* allocate mem */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        : "=b"(mem_hi), "=c"(mem_lo), "=S"(si), "=D"(di)
        : "a"(0x501),
          "b"((uint16_t)(alloc_size >> 16)),
          "c"((uint16_t)(alloc_size & 0xffff))
        : "cc");
    stubinfo.memory_handle = ((uint32_t)si << 16) | di;
    mem_lin = ((uint32_t)mem_hi << 16) | mem_lo;
#else
    rc = dpmi_allocate_memory(alloc_size, &mem_lin, &stubinfo.memory_handle);
    if (rc == -1) {
        stub_debug("dpmi mem alloc failed\n");
        exit(EXIT_FAILURE);
    }
#endif
    mem_base = mem_lin - va;
    stubinfo.mem_base = mem_base;
    stub_debug("mem_lin 0x%lx mem_base 0x%lx\n", mem_lin, mem_base);
#ifdef __GNUC__
    client_memory = __MK_FP(clnt_ds, 0);
#else
    __FP_SEG(client_memory) = clnt_ds;
    __FP_OFF(client_memory) = 0;
#endif
    /* set base */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(7),
          "b"(clnt_entry.selector),
          "c"((uint16_t)(mem_base >> 16)),
          "d"((uint16_t)(mem_base & 0xffff))
        : "cc");
#else
    dpmi_set_segment_base_address(clnt_entry.selector, mem_base);
#endif
    /* set descriptor access rights */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(9), "b"(clnt_entry.selector), "c"(0xc0fb)
        : "cc");
#else
    dpmi_set_descriptor_access_rights(clnt_entry.selector, 0xc0fb);
#endif
    /* set limit */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(8), "b"(clnt_entry.selector),
          "c"(0xffff),
          "d"(0xffff)
        : "cc");
#else
    dpmi_set_segment_limit(clnt_entry.selector, 0xffffffff);
#endif
    /* set base */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(7),
          "b"(clnt_ds),
          "c"((uint16_t)(mem_base >> 16)),
          "d"((uint16_t)(mem_base & 0xffff))
        : "cc");
#else
    dpmi_set_segment_base_address(clnt_ds, mem_base);
#endif
    /* set descriptor access rights */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(9), "b"(clnt_ds), "c"(0xc0f3)
        : "cc");
#else
    dpmi_set_descriptor_access_rights(clnt_ds, 0xc0f3);
#endif
    /* set limit */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(8), "b"(clnt_ds),
          "c"(0xffff),
          "d"(0xffff)
        : "cc");
#else
    dpmi_set_segment_limit(clnt_ds, 0xffffffff);
#endif

    /* create ds alias */
#ifdef __GNUC__
    asm volatile(
        "mov %%ds, %%bx\n"
        "int $0x31\n"
        : "=a"(stubinfo_fs)
        : "a"(0xa));
#else
    stubinfo_fs = dpmi_create_alias_descriptor(__my_ds());
#endif
#ifdef __GNUC__
    stubinfo_mem = _DPMIGetSegmentBaseAddress(stubinfo_fs) +
            (uintptr_t)&stubinfo;
    mem_hi = stubinfo_mem >> 16;
    mem_lo = stubinfo_mem & 0xffff;
    /* set base */
    asm volatile("int $0x31\n"
        :
        : "a"(7), "b"(stubinfo_fs), "c"(mem_hi), "d"(mem_lo)
        : "cc");
#else
    dpmi_get_segment_base_address(stubinfo_fs, &stubinfo_mem);
    stubinfo_mem += (uintptr_t)&stubinfo;
    dpmi_set_segment_base_address(stubinfo_fs, stubinfo_mem);
#endif
    /* set limit */
#ifdef __GNUC__
    asm volatile("int $0x31\n"
        :
        : "a"(8), "b"(stubinfo_fs),
          "c"(0),
          "d"(sizeof(_GO32_StubInfo) - 1)
        : "cc");
#else
    dpmi_set_segment_limit(stubinfo_fs, sizeof(_GO32_StubInfo) - 1);
#endif

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
    stubinfo.stubinfo_ver |= 2;

    stub_debug("Jump to entry...\n");
#ifdef __GNUC__
    asm volatile(
          ".arch i386\n"
          "mov %0, %%fs\n"
          "push %%ds\n"
          "pop %%es\n"
          "mov %1, %%ds\n"
          "ljmpl *%%es:%2\n"
          ".arch i286\n"
        :
        : "a"(stubinfo_fs), "r"(clnt_ds), "m"(clnt_entry)
        : "memory");
#else
    jump_to_entry(stubinfo_fs, clnt_ds);
#endif
    return 0;
}
