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

/* Based on djgpp's stubify.c, original copyrights below: */
/* Copyright (C) 2003 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1999 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/*
 * Note: the previous copyrights are provided only to give credits
 * to previous authors. This file was entirely rewritten and
 * COPYING.DJ doesn't apply to this project.
 * See LICENSE file for an actual license information.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <assert.h>
#include "elf.h"

extern char _binary_stub_exe_end[];
extern char _binary_stub_exe_size[];
extern char _binary_stub_exe_start[];

#define v_printf if(verbose)printf
static int verbose;
static int rmstub;
#define MAX_OVL 5
static char *overlay[MAX_OVL];
static int strip;
static const uint8_t stub_ver = 4;

static int copy_file(const char *ovl, int ofile)
{
  char buf[4096];
  int rbytes;
  int ret = 0;
  int iovfile = open(ovl, O_RDONLY);
  if (iovfile < 0)
  {
    perror(ovl);
    return -1;
  }
  while ((rbytes = read(iovfile, buf, 4096)) > 0)
  {
    int wb;

    wb = write(ofile, buf, rbytes);
    if (wb != rbytes) {
      ret = -1;
      break;
    }
  }
  close(iovfile);
  return ret;
}

static const char *payload_dsc[] = {
  "%s DOS payload",
  "%s/ELF host payload",
  "%s/ELF host debug info",
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static const char *elf_mach(int mach)
{
  switch (mach) {
    case EM_386:
      return "i386";
    case EM_X86_64:
      return "x86_64";
    case EM_ARM:
      return "arm32";
    case EM_AARCH64:
      return "aarch64";
    case EM_RISCV:
      return "risc-v";
  }
  return "unsupported ELF machine type";
}

static const char *elf_id(int fd, long offs)
{
  Elf64_Ehdr ehdr;
  unsigned char *buf = (unsigned char *)&ehdr;
  int rd = pread(fd, buf, sizeof(ehdr), offs);
  if (rd != sizeof(ehdr))
    return "???";
  if (memcmp(buf, ELFMAG, SELFMAG) != 0)
    return "not an ELF";
  switch (buf[EI_CLASS]) {
    case ELFCLASS32: {
      Elf32_Ehdr ehdr32;
      memcpy(&ehdr32, buf, sizeof(ehdr32));
      return elf_mach(ehdr32.e_machine);
    }
    case ELFCLASS64:
      return elf_mach(ehdr.e_machine);
  }
  return "unsupported ELF class";
}

static const char *identify(int num, int fd, long offs)
{
  switch (num) {
    case 0:
      return payload_dsc[num];
    case 1:
    case 2: {
      static char ret[256];
      snprintf(ret, sizeof(ret), payload_dsc[num], elf_id(fd, offs));
      return ret;
    }
  }
  return "???";
}

static int move_tmp(const char *src, const char *dst)
{
  int err = rename(src, dst);
  if (err && errno == EXDEV) {
    int fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      perror("open()");
      return -1;
    }
    copy_file(src, fd);
    close(fd);
    err = unlink(src);
    if (err) {
      perror("unlink()");
      return -1;
    }
  }
  if (err) {
    perror("rename()");
    return -1;
  }
  return 0;
}

static int coff2exe(const char *fname, const char *oname, int info)
{
  char ibuf[1024], ibuf0[256];
  int has_o0 = 0;
#define IPRINTF(...) \
    snprintf(ibuf + strlen(ibuf), sizeof(ibuf) - strlen(ibuf), __VA_ARGS__)
  int ifile;
  int ofile;
  char buf[4096];
  int rbytes;
  long coffset = 0;
  unsigned char mzhdr_buf[0x40];
  uint32_t coff_file_size = 0;
  int rmoverlay = 0;
  int can_copy_ovl = 0;
  int i;
  int ret = 0;
  char tmpl[] = "/tmp/djstub_XXXXXX";
  const uint32_t stub_size = _binary_stub_exe_end - _binary_stub_exe_start;

  ibuf[0] = '\0';
  ibuf0[0] = '\0';

  ifile = open(fname, O_RDONLY);
  if (ifile < 0)
  {
    perror("open()");
    return -1;
  }

  while (1)
  {
    lseek(ifile, coffset, SEEK_SET);
    read(ifile, buf, 0x40);
    if (buf[0] == 'M' && buf[1] == 'Z') /* stubbed already, skip stub */
    {
      if (buf[8] == 4 && buf[9] == 0)  // lfanew
      {
        uint32_t offs;
        uint16_t flags = 0;
        int dyn = 0;
        int stub_v4 = (buf[0x3b] >= 4 && buf[0x3b] < 0x20);

        if (stub_v4) {
          memcpy(&flags, &buf[0x2c], sizeof(flags));
          if (flags & 0x80)
            dyn++;
        }
        memcpy(&offs, &buf[0x3c], sizeof(offs));
        coffset = offs;
        memcpy(&coff_file_size, &buf[0x1c], sizeof(coff_file_size));
        memcpy(mzhdr_buf, buf, sizeof(mzhdr_buf));
        can_copy_ovl++;
        if (rmstub || strip)
          rmoverlay++;

        if (info) {
          strcat(ibuf, "dj64 file format\n");
          if (dyn)
            strcat(ibuf, "DOS payload dynamic\n");
        }
        if (info || strip) {
          int cnt = 0;
          uint32_t sz = 0;
          for (i = 0x1c; i < 0x30; i += 4) {
            uint32_t sz0;
            memcpy(&sz0, &buf[i], sizeof(sz0));
            if (!sz0) {
              if (strip && sz) {
                coff_file_size = offs - sz - coffset;
                memset(&mzhdr_buf[i - 4], 0, 4);
              }
              break;
            }
            sz = sz0;
            if (!cnt)
              has_o0++;
            if (info)
              IPRINTF("Overlay %i (%s) at %i, size %i\n", cnt,
                  identify(cnt + dyn, ifile, offs),
                  offs, sz);
            offs += sz;
            cnt++;
          }
          if (stub_v4) {
            if (buf[0x2e])
              IPRINTF("Overlay name: %.12s\n", buf + 0x2e);
            IPRINTF("Stub version: %i\n", buf[0x3b]);
            IPRINTF("Stub flags: 0x%04x\n", flags);
          }
        }
      }
      else
      {
        int blocks = (unsigned char)buf[4] + (unsigned char)buf[5] * 256;
        int partial = (unsigned char)buf[2] + (unsigned char)buf[3] * 256;
        if (info)
          strcat(ibuf, "exe/djgpp file format\n");
        coffset += blocks * 512;
        if (partial)
          coffset += partial - 512;
      }
    }
    else if (buf[0] == 0x4c && buf[1] == 0x01) /* it's a COFF */
    {
      if (info) {
        if (has_o0)
          strcpy(ibuf0, "i386/COFF");
        else
          IPRINTF("COFF payload at %li\n", coffset);
      }
      break;
    }
    else if (buf[0] == 0x7f && buf[1] == 0x45 &&
                buf[2] == 0x4c && buf[3] == 0x46) /* it's an ELF */
    {
      if (info) {
        if (has_o0)
          snprintf(ibuf0, sizeof(ibuf0), "%s/ELF", elf_id(ifile, coffset));
        else
          IPRINTF("ELF payload for %s at %li\n", elf_id(ifile, coffset), coffset);
      }
      break;
    }
    else
    {
      fprintf(stderr, "Warning: input file is neither COFF nor stubbed COFF\n");
      break;
    }
  }

  if (!coff_file_size)
    coff_file_size = lseek(ifile, 0, SEEK_END) - coffset;
  lseek(ifile, coffset, SEEK_SET);
  if (can_copy_ovl) {
    /* copy entire overlay info except 0x3c */
    memcpy(_binary_stub_exe_start + 0x1c, mzhdr_buf + 0x1c, 0x3c - 0x1c);
  } else {
    /* assume there is just 1 overlay */
    memcpy(_binary_stub_exe_start + 0x1c, &coff_file_size,
          sizeof(coff_file_size));
  }
  _binary_stub_exe_start[0x3b] = stub_ver;
  memcpy(_binary_stub_exe_start + 0x3c, &stub_size, sizeof(stub_size));

  if (info) {
    if (has_o0)
      printf(ibuf, ibuf0);
    else
      printf("%s", ibuf);
    close(ifile);
    return 0;
  }

  if (oname) {
    ofile = open(oname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (ofile < 0) {
      perror("open()");
      return -1;
    }
  } else {
    ofile = mkstemp(tmpl);
    if (ofile < 0) {
      perror("mkstemp()");
      return -1;
    }
  }

  if (!rmstub)
    write(ofile, _binary_stub_exe_start,
        _binary_stub_exe_end-_binary_stub_exe_start);

  /* copy either entire payload, or, in case of rmoverlay, only COFF */
  while (coff_file_size > 0 && (rbytes=read(ifile, buf, 4096)) > 0)
  {
    int wb;

    if (rmoverlay && rbytes > coff_file_size)
      rbytes = coff_file_size;

    wb = write(ofile, buf, rbytes);
    if (rmoverlay)
      coff_file_size -= rbytes;
    if (wb != rbytes)
    {
      perror("write()");
      close(ifile);
      close(ofile);
      unlink(oname);
      return -1;
    }
  }
  close(ifile);
  close(ofile);

  if (!oname)
    ret = move_tmp(tmpl, fname);

  return ret;
}

static void print_help(void)
{
  fprintf(stderr, "Usage: stubify [-v] [-l <overlay>] [-o <out_file>] <program>\n\n"
	  "<program> may be COFF or stubbed .exe.\n\n"
	  "Options:\n"
	  "-v -> verbose\n"
	  "-i -> display file info\n"
	  "-s -> strip last overlay\n"
	  "-r -> remove stub (and overlay, if any)\n"
	  "-l <file_name> -> link in <file_name> file as an overlay\n"
	  "-n <name> -> write <name> into an overlay info\n"
	  "-o <name> -> write output into <name>\n"
	  "-f <flags> -> write <flags> into an overlay info\n"
	  "-g -> generate a new file\n"
	  "\nNote: -g is useful only for debugging, as the stub is being\n"
	  "customized for a particular program and its overlay.\n"
  );
}

int main(int argc, char **argv)
{
  int generate = 0;
  char *oname = NULL;
  int info = 0;
  int c;
  int noverlay = 0;
  const char *ovname = NULL;
  uint16_t stub_flags = 0;

  if (_binary_stub_exe_start[0] != 'M' || _binary_stub_exe_start[1] != 'Z' ||
        _binary_stub_exe_start[8] != 4 || _binary_stub_exe_start[9] != 0) {
    fprintf(stderr, "stub corrupted, bad build\n");
    return EXIT_FAILURE;
  }

  while ((c = getopt(argc, argv, "virsgl:o:n:f:")) != -1)
  {
    switch (c) {
    case 'v':
      verbose = 1;
      break;
    case 'i':
      info = 1;
      break;
    case 'r':
      rmstub = 1;
      break;
    case 's':
      strip = 1;
      break;
    case 'g':
      generate = 1;
      break;
    case 'l':
      assert(noverlay < MAX_OVL);
      overlay[noverlay++] = optarg;
      break;
    case 'n':
      ovname = optarg;
      break;
    case 'f':
      stub_flags |= strtol(optarg, NULL, 0);
      break;
    case 'o':
      oname = optarg;
      break;
    default:
      fprintf(stderr, "Unknow option: %c\n", c);
      print_help();
      return 1;
    }
  }

  v_printf("stubify for dj64 executables, copyright (C) 2023 stsp\n");
  if (generate)
  {
    int ofile;
    int i;
    const uint32_t stub_size = _binary_stub_exe_end - _binary_stub_exe_start;

    if (!oname) {
      fprintf(stderr, "djstubify: -o missing\n");
      return EXIT_FAILURE;
    }
    if (noverlay) {
      struct stat sb;
      /* store overlay sizes in overlay info */
      for (i = 0; i < noverlay; i++) {
        int rc = stat(overlay[i], &sb);
        if (rc == -1) {
          fprintf(stderr, "failed to stat %s: %s\n", overlay[i],
              strerror(errno));
          return EXIT_FAILURE;
        }
        memcpy(_binary_stub_exe_start + 0x1c + i * 4, &sb.st_size, 4);
      }
    }
    memcpy(_binary_stub_exe_start + 0x2c, &stub_flags, sizeof(stub_flags));
    if (ovname) {
      strncpy(_binary_stub_exe_start + 0x2e, ovname, 12);
      _binary_stub_exe_start[0x2e + 12] = '\0';
    }
    _binary_stub_exe_start[0x3b] = stub_ver;
    memcpy(_binary_stub_exe_start + 0x3c, &stub_size, sizeof(stub_size));

    ofile = open(oname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (ofile < 0)
    {
      fprintf(stderr, "Cannot open output file to generate\n");
      perror("open()");
      return 1;
    }
    v_printf("stubify: generate %s\n", oname);

    write(ofile, _binary_stub_exe_start,
        _binary_stub_exe_end-_binary_stub_exe_start);

    for (i = 0; i < noverlay; i++)
    {
      int rc = copy_file(overlay[i], ofile);
      if (rc < 0)
      {
        fprintf(stderr, "failed to copy overlays\n");
        close(ofile);
        unlink(oname);
        return EXIT_FAILURE;
      }
    }

    close(ofile);
  }
  else
  {
    int err;

    if (argc < 2)
    {
      print_help();
      return 1;
    }

    err = coff2exe(argv[argc - 1], oname, info);
    if (err)
      return 1;
  }

  return 0;
}
