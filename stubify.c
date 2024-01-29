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

extern char _binary_stub_exe_end[];
extern char _binary_stub_exe_size[];
extern char _binary_stub_exe_start[];

#define v_printf if(verbose)printf
static int verbose;
static int rmstub;
static char *overlay;

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

static void coff2exe(char *fname, char *oname)
{
  char ifilename[256];
  char ofilename[256];
  int ifile;
  int ofile;
  char *ofname, *ofext;
  char buf[4096];
  int rbytes, used_temp = 0;
  long coffset=0;
  unsigned char mzhdr_buf[0x40];
  uint32_t coff_file_size = 0;
  int rmoverlay = 0;
  int can_copy_ovl = 0;

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
      if (buf[8] == 4)  // lfanew
      {
        uint32_t offs;
        memcpy(&offs, &buf[0x3c], sizeof(offs));
        coffset = offs;
        memcpy(&coff_file_size, &buf[0x1c], sizeof(coff_file_size));
        memcpy(mzhdr_buf, buf, sizeof(mzhdr_buf));
        can_copy_ovl++;
        if (rmstub || overlay)
          rmoverlay++;
      }
      else
      {
        int blocks = (unsigned char)buf[4] + (unsigned char)buf[5] * 256;
        int partial = (unsigned char)buf[2] + (unsigned char)buf[3] * 256;
        coffset += blocks * 512;
        if (partial)
          coffset += partial - 512;
      }
    }
    else if (buf[0] == 0x4c && buf[1] == 0x01) /* it's a COFF */
    {
      break;
    }
    else if (buf[0] == 0x7f && buf[1] == 0x45 &&
                buf[2] == 0x4c && buf[3] == 0x46) /* it's an ELF */
    {
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
  if (overlay) {
    struct stat sb;
    /* store overlay sizes in overlay info */
    memcpy(_binary_stub_exe_start + 0x1c, &coff_file_size,
          sizeof(coff_file_size));
    stat(overlay, &sb);
    memcpy(_binary_stub_exe_start + 0x20, &sb.st_size, 4);
  } else if (can_copy_ovl) {
    /* copy entire overlay info except 0x3c */
    memcpy(_binary_stub_exe_start + 0x1c, mzhdr_buf + 0x1c, 0x3c - 0x1c);
  } else {
    /* assume there is just 1 overlay */
    memcpy(_binary_stub_exe_start + 0x1c, &coff_file_size,
          sizeof(coff_file_size));
  }

  lseek(ifile, coffset, SEEK_SET);

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

  if (overlay)
  {
    int rc = copy_ovl(overlay, ofile);
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
	  "-r -> remove stub (and overlay, if any)\n"
	  "-l <file_name> -> link in <file_name> file as an overlay\n"
	  "-g <file_name> -> write a stub alone into a file\n"
	  "\nNote: -g is useful only for debugging, as the stub is being\n"
	  "customized for a particular program and its overlay.\n"
  );
}

int main(int argc, char **argv)
{
  char *generate = NULL;
  char *oname = NULL;

  while (argc > 1 && argv[1][0] == '-')
  {
    if (strcmp(argv[1], "-v")==0)
    {
      verbose = 1;
      argv++;
      argc--;
    }
    if (strcmp(argv[1], "-r")==0)
    {
      rmstub = 1;
      argv++;
      argc--;
    }
    else if (strcmp(argv[1], "-g")==0)
    {
      if (argc < 2)
      {
	fprintf(stderr, "-g option requires file name\n");
	return 1;
      }
      generate = argv[2];
      argv += 2;
      argc -= 2;
    }
    else if (strcmp(argv[1], "-l")==0)
    {
      if (argc < 2)
      {
	fprintf(stderr, "-l option requires file name\n");
	return 1;
      }
      overlay = argv[2];
      argv += 2;
      argc -= 2;
    }
    else if (strcmp(argv[1], "-o")==0)
    {
      if (argc < 2)
      {
	fprintf(stderr, "-o option requires file name\n");
	return 1;
      }
      oname = argv[2];
      argv += 2;
      argc -= 2;
    }
    else
    {
      fprintf(stderr, "Unknow option: %s\n", argv[1]);
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
