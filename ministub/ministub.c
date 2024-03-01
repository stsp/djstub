#include <stdio.h>
#include <fcntl.h>

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

static void enter_stub(int fd, unsigned sel, unsigned off,
    int argc, char *argv[], char *envp[])
{
  asm(
    "mov eax, [ebp+8]\n"     // fd
    "mov ecx, [ebp+20]\n"    // argc
    "mov edx, [ebp+24]\n"    // argv
    "mov esi, [ebp+28]\n"    // envp
    "push dword [ebp+12]\n"  // sel
    "push dword [ebp+16]\n"  // off
    "call far [ss:esp]\n"
    "add esp, 8\n"
  );
}

int main(int argc, char *argv[], char *envp[])
{
  static const char *ext_nm = "DJ64STUB";
  unsigned short sel, off;
  int fd;
  int err = DPMIQueryExtension(&sel, &off, ext_nm);

  if (argc == 0) {
    puts("no env");
    return 1;
  }
  if (err) {
    printf("%s unsupported (%x)\n", ext_nm, err);
    return 1;
  }
  fd = open(argv[0], O_RDONLY);
  if (fd == -1) {
    printf("unable to open %s\n", argv[0]);
    return 1;
  }
  enter_stub(fd, sel, off, argc, argv, envp);
  close(fd);
  puts("stub returned");
  return 0;
}
