.data
.globl _binary_fullstub_exe_start
#ifdef __ELF__
.size _binary_fullstub_exe_start,0
#endif
_binary_fullstub_exe_start:
.incbin "fullstub.exe"
.globl _binary_fullstub_exe_end
#ifdef __ELF__
.size _binary_fullstub_exe_end,0
#endif
_binary_fullstub_exe_end:

#ifdef __ELF__
.section .note.GNU-stack,"",%progbits
#endif
