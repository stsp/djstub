.data
.globl _binary_ministub_exe_start
.size _binary_ministub_exe_start,0
_binary_ministub_exe_start:
.incbin "ministub.exe"
.globl _binary_ministub_exe_end
.size _binary_ministub_exe_end,0
_binary_ministub_exe_end:

#ifdef __ELF__
.section .note.GNU-stack,"",%progbits
#endif
