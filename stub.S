.data
.globl _binary_stub_exe_start
#ifdef __ELF__
.size _binary_stub_exe_start,0
#endif
_binary_stub_exe_start:
.incbin "stub.exe"
.globl _binary_stub_exe_end
#ifdef __ELF__
.size _binary_stub_exe_end,0
#endif
_binary_stub_exe_end:

#ifdef __ELF__
.section .note.GNU-stack,"",%progbits
#endif
