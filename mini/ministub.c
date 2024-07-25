/*
 *  mini stub for dj64.
 *  Copyright (C) 2024,  @stsp
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
#include <string.h>

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

int __dpmi_int(int intno, __dpmi_int_regs* regs);

int DPMIQueryExtension(unsigned short *sel, unsigned short *off,
    const char *name)
{
  asm(
  "push es\n"
  "mov eax, 0x168a\n"
  "mov esi, [ebp+16]\n" // name
  "int 0x2f\n"
  "mov ebx, [ebp+8]\n"  // sel
  "mov [ebx], es\n"
  "mov ebx, [ebp+12]\n"
  "mov [ebx], edi\n"
  "pop es\n"
  "xor ah, ah\n"
  );
}

static void enter_stub(unsigned sel, unsigned off,
    int argc, char *argv[], int envc, char *envp[], unsigned psp)
{
  asm(
    "mov eax, [ebp+32]\n"    // psp
    "mov ecx, [ebp+16]\n"    // argc
    "mov edx, [ebp+20]\n"    // argv
    "mov ebx, [ebp+24]\n"    // envc
    "mov esi, [ebp+28]\n"    // envp
    "push dword [ebp+8]\n "  // sel
    "push dword [ebp+12]\n"  // off
    "call far [ss:esp]\n"
    "add esp, 8\n"
  );
}

extern void* __dpmi_psp;
extern void* __dpmi_env;
static unsigned psp;

int main(int argc, char *argv[])
{
  static const char *ext_nm = "DJ64STUB";
  unsigned char *env = __dpmi_env;
  char **envp;
  unsigned short sel, off;
  int envc, i, letter;
  int err;
  __dpmi_int_regs regs;

  if (argc == 0) {
    puts("no env");
    return 1;
  }
  for (envc = i = letter = 0;; i++) {
    if (env[i] == '\0') {
      letter = 0;
      if (env[i + 1] == '\0')
        break;
    } else {
      if (!letter) {
        letter = 1;
        envc++;
      }
    }
  }
  envp = malloc((envc + 1) * sizeof(char *));
  envp[envc] = NULL;
  if (envc) {
    int envc2;
    for (envc2 = i = letter = 0;; i++) {
      if (env[i] == '\0') {
        letter = 0;
        if (env[i + 1] == '\0')
          break;
      } else {
        if (!letter) {
          letter = 1;
          envp[envc2++] = &env[i];
        }
      }
    }
  }
  asm(
    "mov eax, 0\n"    // alloc desc for PSP
    "mov ecx, 1\n"    // 1 desc
    "int 31h\n"
    "mov [_psp], ax\n"
    "mov eax, 7\n"    // set base
    "mov bx, [_psp]\n"
    "extern  ___dpmi_psp\n"
    "mov cx, [___dpmi_psp + 2]\n"
    "mov dx, [___dpmi_psp]\n"
    "int 31h\n"
    "mov eax, 8\n"    // set limit
    "mov bx, [_psp]\n"
    "mov cx, 0\n"
    "mov dx, 0ffh\n"  // to PSP size
    "int 31h\n"
  );
  err = DPMIQueryExtension(&sel, &off, ext_nm);
  if (err) {
    printf("%s unsupported (%x)\n", ext_nm, err);
    return 1;
  }
  memset(&regs, 0, sizeof(regs));
  /* nuke out initial stub as it is no longer needed */
  regs.eax = 0x4a00;
  regs.ebx = 0x10;  // leave only PSP
  regs.es = (unsigned)__dpmi_psp >> 4;
  __dpmi_int(0x21, &regs);
  enter_stub(sel, off, argc, argv, envc, envp, psp);
  puts("stub returned");
  return 0;
}
