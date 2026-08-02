import ctypes

# Constants
EI_NIDENT = 16

# File identification bytes
EI_MAG0 = 0
ELFMAG0 = 0x7f

EI_MAG1 = 1
ELFMAG1 = ord('E')

EI_MAG2 = 2
ELFMAG2 = ord('L')

EI_MAG3 = 3
ELFMAG3 = ord('F')

ELFMAG = b"\x7fELF"
SELFMAG = 4

EI_CLASS = 4
ELFCLASSNONE = 0
ELFCLASS32 = 1
ELFCLASS64 = 2
ELFCLASSNUM = 3

# Architecture defines
EM_386 = 3
EM_ARM = 40
EM_X86_64 = 62
EM_AARCH64 = 183
EM_RISCV = 243

class Elf32_Ehdr(ctypes.Structure):
    e_ident: ctypes.Array
    e_type: int
    e_machine: int
    e_version: int
    e_entry: int
    e_phoff: int
    e_shoff: int
    e_flags: int
    e_ehsize: int
    e_phentsize: int
    e_phnum: int
    e_shentsize: int
    e_shnum: int
    e_shstrndx: int
    _fields_ = [
        ("e_ident", ctypes.c_ubyte * EI_NIDENT),
        ("e_type", ctypes.c_uint16),
        ("e_machine", ctypes.c_uint16),
        ("e_version", ctypes.c_uint32),
        ("e_entry", ctypes.c_uint32),
        ("e_phoff", ctypes.c_uint32),
        ("e_shoff", ctypes.c_uint32),
        ("e_flags", ctypes.c_uint32),
        ("e_ehsize", ctypes.c_uint16),
        ("e_phentsize", ctypes.c_uint16),
        ("e_phnum", ctypes.c_uint16),
        ("e_shentsize", ctypes.c_uint16),
        ("e_shnum", ctypes.c_uint16),
        ("e_shstrndx", ctypes.c_uint16),
    ]

class Elf64_Ehdr(ctypes.Structure):
    e_ident: ctypes.Array
    e_type: int
    e_machine: int
    e_version: int
    e_entry: int
    e_phoff: int
    e_shoff: int
    e_flags: int
    e_ehsize: int
    e_phentsize: int
    e_phnum: int
    e_shentsize: int
    e_shnum: int
    e_shstrndx: int
    _fields_ = [
        ("e_ident", ctypes.c_ubyte * EI_NIDENT),
        ("e_type", ctypes.c_uint16),
        ("e_machine", ctypes.c_uint16),
        ("e_version", ctypes.c_uint32),
        ("e_entry", ctypes.c_uint64),
        ("e_phoff", ctypes.c_uint64),
        ("e_shoff", ctypes.c_uint64),
        ("e_flags", ctypes.c_uint32),
        ("e_ehsize", ctypes.c_uint16),
        ("e_phentsize", ctypes.c_uint16),
        ("e_phnum", ctypes.c_uint16),
        ("e_shentsize", ctypes.c_uint16),
        ("e_shnum", ctypes.c_uint16),
        ("e_shstrndx", ctypes.c_uint16),
    ]
