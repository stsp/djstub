# djstub
djstub project provides a dj64/dj32-compatible and go32-compatible
stub that support COFF, PE and ELF payloads.
Its primary target is [dj64dev](https://github.com/stsp/dj64dev)
suite, but it can also work with djgpp-built
executables or plain COFF and ELF files.

## what it does?
- It can stubify or re-stub COFF and PE executables. See `djstubify`.
- It can link the ELF binaries into a dj64/dj32 executables. See `djlink`.
- It can strip the debug info from dj64/dj32 executables. See `djstrip`.

## usage examples
```
$ djstubify -i comcom64.exe
dj64 file format
Overlay 0 (i386/ELF DOS payload) at 23368, size 30548
Overlay 1 (x86_64/ELF host payload) at 53916, size 87048
Overlay 2 (x86_64/ELF host debug info) at 140964, size 174936
Overlay name: comcom64.exe
Stub version: 4
Stub flags: 0x0b07
```

```
$ djlink -d dosemu_hello.exe.dbg libtmp.so -n hello.exe -o hello.exe tmp.elf
```

These examples are [documented](https://github.com/stsp/dj64dev/blob/master/README.md)
in a dj64dev project.
