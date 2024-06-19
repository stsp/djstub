# djstub
djstub project provides a dj64-compatible and go32-compatible
stubs that support COFF, PE and ELF payloads.
Its primary target is [dj64dev](https://github.com/stsp/dj64dev)
suite, but it can also work with djgpp-built
executables.

## what it does?
- It can stubify or re-stub COFF and PE executables. See `djstubify` for that task.
- It can link the ELF binaries into a dj64 executables. See `djlink` for that task.
- It can strip the debug info from dj64 executables. See `djstrip` for that task.

## what stubs does it provide?
There are 2 stubs available: [full](https://github.com/stsp/djstub/tree/main/full),
which has all the loaders within, and
[mini](https://github.com/stsp/djstub/tree/main/mini),
which relies on a `"DJ64STUB"` DPMI extension, which is
supposed to have the same loaders inside DPMI host.

Full stub requires
[gcc-ia16](https://gitlab.com/tkchia/gcc-ia16)
to build, and is therefore currently not maintained.
Mini stub uses [SmallerC](https://github.com/alexfru/SmallerC)
and is actively maintained. If you want to use this
project outside of dj64dev suite, you may need full
stub. Otherwise, mini stub is enough.

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
