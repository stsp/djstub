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
static int noverlay;
static const char *ovname;
static int info;
static int strip;
static uint16_t stub_flags;
static const uint8_t stub_ver = 4;

static int copy_ovl(const char *ovl, int ofile)
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

static void coff2exe(char *fname, char *oname)
{
  char ibuf[1024], ibuf0[256];
  int has_o0 = 0;
#define IPRINTF(...) \
    snprintf(ibuf + strlen(ibuf), sizeof(ibuf) - strlen(ibuf), __VA_ARGS__)
  char ifilename[256];
  char ofilename[256];
  int ifile;
  int ofile;
  char *ofname, *ofext;
  char buf[4096];
  int rbytes, used_temp = 0;
  long coffset = 0;
  unsigned char mzhdr_buf[0x40];
  uint32_t coff_file_size = 0;
  int rmoverlay = 0;
  int can_copy_ovl = 0;
  int i;
  const uint32_t stub_size = _binary_stub_exe_end - _binary_stub_exe_start;

  ibuf[0] = '\0';
  ibuf0[0] = '\0';
  strcpy(ifilename, fname);
  strcpy(ofilename, oname);
  ofext = 0;
  for (ofname=ofilename; *ofname; ofname++)
  {
    if (strchr("/\\:", *ofname))
      ofext = 0;
    if (*ofname == '.')
      ofext = ofname;
  }
  if (ofext == 0)
    ofext = ofilename + strlen(ofilename);
  strcpy(ofext, ".exe");
  if (access(ofilename, 0) == 0)
    for (ofile=0; ofile<999; ofile++)
    {
      used_temp = 1;
      sprintf(ofext, ".%03d", ofile);
      if (access(ofilename, 0))
	break;
    }
  else
    used_temp = 0;

  ifile = open(ifilename, O_RDONLY);
  if (ifile < 0)
  {
    perror(fname);
    return;
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
        memcpy(&offs, &buf[0x3c], sizeof(offs));
        coffset = offs;
        memcpy(&coff_file_size, &buf[0x1c], sizeof(coff_file_size));
        memcpy(mzhdr_buf, buf, sizeof(mzhdr_buf));
        can_copy_ovl++;
        if (rmstub || noverlay || strip)
          rmoverlay++;

        if (info)
          strcat(ibuf, "dj64 file format\n");
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
                  identify(cnt, ifile, offs),
                  offs, sz);
            offs += sz;
            cnt++;
          }
        }
        if (info && buf[0x30])
          IPRINTF("Overlay name: %.12s\n", buf + 0x30);
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
  if (noverlay) {
    struct stat sb;
    /* store overlay sizes in overlay info */
    memcpy(_binary_stub_exe_start + 0x1c, &coff_file_size,
          sizeof(coff_file_size));
    for (i = 0; i < noverlay; i++) {
      int rc = stat(overlay[i], &sb);
      if (rc == -1) {
        fprintf(stderr, "failed to stat %s: %s\n", overlay[i],
            strerror(errno));
        close(ifile);
        return;
      }
      memcpy(_binary_stub_exe_start + 0x20 + i * 4, &sb.st_size, 4);
    }
  } else if (can_copy_ovl) {
    /* copy entire overlay info except 0x3c */
    memcpy(_binary_stub_exe_start + 0x1c, mzhdr_buf + 0x1c, 0x3c - 0x1c);
  } else {
    /* assume there is just 1 overlay */
    memcpy(_binary_stub_exe_start + 0x1c, &coff_file_size,
          sizeof(coff_file_size));
  }
  memcpy(_binary_stub_exe_start + 0x2c, &stub_flags, sizeof(stub_flags));
  if (ovname) {
    strncpy(_binary_stub_exe_start + 0x2e, ovname, 12);
    _binary_stub_exe_start[0x2e + 12] = '\0';
  }
  _binary_stub_exe_start[0x3b] = stub_ver;
  memcpy(_binary_stub_exe_start + 0x3c, &stub_size, sizeof(stub_size));

  if (info) {
    if (has_o0)
      printf(ibuf, ibuf0);
    else
      printf("%s", ibuf);
    close(ifile);
    return;
  }

  ofile = open(ofilename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
  if (ofile < 0)
  {
    perror(ofilename);
    return;
  }
  v_printf("stubify: %s -> %s", ifilename, ofilename);
  if (used_temp)
  {
    strcpy(ifilename, ofilename);
    strcpy(ofext, ".exe");
    v_printf(" -> %s", ofilename);
  }
  v_printf("\n");

  if (!rmstub)
    write(ofile, _binary_stub_exe_start,
        _binary_stub_exe_end-_binary_stub_exe_start);

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
      perror(ofilename);
      close(ifile);
      close(ofile);
      unlink(ofilename);
      exit(1);
    }
  }
  close(ifile);

  for (i = 0; i < noverlay; i++)
  {
    int rc = copy_ovl(overlay[i], ofile);
    if (rc < 0)
    {
      perror(ofilename);
      close(ofile);
      unlink(ofilename);
      exit(1);
    }
  }
  close(ofile);

  if (used_temp)
  {
    unlink(ofilename);
    if (rename(ifilename, ofilename))
    {
      fprintf(stderr, "rename of %s to %s failed.\n", ifilename, ofilename);
      perror("The error was");
    }
  }
}

static void print_help(void)
{
  fprintf(stderr, "Usage: stubify [-v] [-l <overlay>] <program>\n\n"
	  "<program> may be COFF or stubbed .exe, and may be COFF with .exe extension.\n"
	  "Resulting file will have .exe\n\n"
	  "Options:\n"
	  "-v -> verbose\n"
	  "-i -> display file info\n"
	  "-s -> strip last overlay\n"
	  "-r -> remove stub (and overlay, if any)\n"
	  "-l <file_name> -> link in <file_name> file as an overlay\n"
	  "-n <name> -> write <name> into an overlay info\n"
	  "-f <flags> -> write <flags> into an overlay info\n"
	  "-g <file_name> -> write a stub alone into a file\n"
	  "\nNote: -g is useful only for debugging, as the stub is being\n"
	  "customized for a particular program and its overlay.\n"
  );
}

int main(int argc, char **argv)
{
  char *generate = NULL;
  char *oname = NULL;
  int c;

  while ((c = getopt(argc, argv, "virsg:l:o:n:f:")) != -1)
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
      generate = optarg;
      break;
    case 'l':
      assert(noverlay < MAX_OVL);
      overlay[noverlay++] = optarg;
      break;
    case 'n':
      ovname = optarg;
      break;
    case 'f':
      stub_flags = strtol(optarg, NULL, 0);
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
    char ofilename[256], *ofname, *ofext=0;
    int ofile;

    strcpy(ofilename, generate);
    for (ofname=ofilename; *ofname; ofname++)
    {
      if (strchr("/\\:", *ofname))
	ofext = 0;
      if (*ofname == '.')
	ofext = ofname;
    }
    if (ofext == 0)
      ofext = ofilename + strlen(ofilename);
    strcpy(ofext, ".exe");

    ofile = open(ofilename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (ofile < 0)
    {
      fprintf(stderr, "Cannot open output file to generate\n");
      perror(ofilename);
      return 1;
    }
    v_printf("stubify: generate %s\n", generate);

    write(ofile, _binary_stub_exe_start,
        _binary_stub_exe_end-_binary_stub_exe_start);
    close(ofile);
  }
  else
  {
    if (argc < 2)
    {
      print_help();
      return 1;
    }

    coff2exe(argv[argc - 1], oname ?: argv[argc - 1]);
  }

  return 0;
}
