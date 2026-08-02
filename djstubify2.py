#!/usr/bin/env python3

#  go32-compatible COFF, PE32 and ELF loader stub.
#  Copyright (C) 2022 - 2026,  stsp <stsp@users.sourceforge.net>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

import getopt
import ctypes
import struct
import os
import shutil
import sys
import tempfile
from enum import Enum
import elf

_binary_stub_exe_start: bytearray
_binary_stub_exe_size: int

verbose: bool = False
rmstub: bool = False
overlay: list[str] = []
strip: bool = False
stub_ver: int = 8
version: int = 9

def v_printf(msg: str):
    global verbose
    if verbose:
        print(msg)

class ObjType(Enum):
    OT_COFF = 0
    OT_DJ32 = 1
    OT_E32 = 2
    OT_E64 = 3
    OT_D64 = 4
    OT_D32 = 5
    OT_MAX = 6

payload_dsc: list[str] = [
    "COFF DOS payload",
    "ELF (dj32) DOS payload",
    "ELF (dj64) DOS payload",
    "ELF host payload",
    "ELF debug info",
    "ELF (dj32) debug info",
]

def elf_mach(mach: int) -> str:
    match mach:
        case _ as m if m == elf.EM_386:
            return "i386"
        case _ as m if m == elf.EM_X86_64:
            return "x86_64"
        case _ as m if m == elf.EM_ARM:
            return "arm"
        case _ as m if m == elf.EM_AARCH64:
            return "aarch64"
        case _ as m if m == elf.EM_RISCV:
            return "riscv"
    return "unsupported ELF machine type"

def elf_id(f, offs: int) -> str:
    f.seek(offs)
    buf: bytearray = f.read(ctypes.sizeof(elf.Elf64_Ehdr))
    if len(bytearray(buf)) != ctypes.sizeof(elf.Elf64_Ehdr):
        return "???"
    if memoryview(buf)[0:4] != elf.ELFMAG:
        return "Not an ELF"
    match buf[elf.EI_CLASS]:
        case _ as c if c == elf.ELFCLASS32:
            elf32_ehdr: elf.Elf32_Ehdr = elf.Elf32_Ehdr.from_buffer_copy(buf)
            return elf_mach(elf32_ehdr.e_machine)
        case _ as c if c == elf.ELFCLASS64:
            elf64_ehdr: elf.Elf64_Ehdr = elf.Elf64_Ehdr.from_buffer_copy(buf)
            return elf_mach(elf64_ehdr.e_machine)
    return "unsupported ELF class"

def identify(num: int, f, offs: int) -> str:
    if num == ObjType.OT_COFF.value or num >= ObjType.OT_MAX.value:
        return "???"
    return f"{elf_id(f, offs)}/{payload_dsc[num]}"

def find_idx(type_val: int, buf: bytes) -> int:
    type_map: int = int.from_bytes(memoryview(buf)[0x36:0x38], byteorder="little")
    cnt: int = 0
    if type_val == 0 and type_map == 0:
        return 0
    while type_map:
        t:int = type_map & 0xf
        if t == type_val:
            return cnt
        type_map >>= 4
        cnt += 1
    return -1

def find_size(type_val: int, buf: bytes) -> int:
    idx: int = find_idx(type_val, buf)
    if idx == -1:
        return -1
    return int.from_bytes(memoryview(buf)[0x1c + idx * 4 : 0x1c + idx * 4 + 4], byteorder="little")

def find_offs(type_val: int, buf: bytes) -> int:
    type_map: int
    offs: int = 0
    cnt: int = 0
    type_map = int.from_bytes(memoryview(buf)[0x36:0x38], byteorder="little")
    if type_val == 0 and type_map == 0:
        return 0
    while type_map:
        t: int = type_map & 0xf
        if t == type_val:
            return offs
        offs += int.from_bytes(memoryview(buf)[0x1c + cnt * 4 : 0x1c + cnt * 4 + 4], byteorder="little")
        type_map >>= 4
        cnt += 1
    return -1

def write_u16(buf, offset, value):
    memoryview(buf)[offset : offset + 2] = value.to_bytes(2, byteorder='little')

def write_u32(buf, offset, value):
    memoryview(buf)[offset : offset + 4] = value.to_bytes(4, byteorder='little')

def coff2exe(fname: str, oname: str, info: int = 0) -> int:
    global stub_ver

    ibuf = []

    def iprintf(fmt, *args):
        ibuf.append(fmt % args)

    try:
        ifile = open(fname, "rb")
    except IOError as e:
        print(f"open({fname}): {e.strerror}", file=sys.stderr)
        return -1

    coffset = 0
    mzhdr_buf = bytearray(0x40)
    coff_file_size = 0
    rmoverlay = 0
    can_copy_ovl = 0
    stub_v = 0
    flags = 0

    while True:
        ifile.seek(coffset, os.SEEK_SET)
        buf = ifile.read(0x40)
        if len(buf) < 0x40:
            assert False, "Unable to read 0x40 header bytes"

        mv = memoryview(buf)

        if buf[0] == ord('M') and buf[1] == ord('Z'):
            if buf[8] == 4 and buf[9] == 0:  # lfanew
                dyn = 0
                dj32 = 0
                elf = 0
                name = b""

                stub_v = buf[0x3b]
                if stub_v < 8:
                    print(f"stub too old: {stub_v}", file=sys.stderr)
                    ifile.close()
                    return -1

                flags = int.from_bytes(mv[0x38:0x3a], byteorder="little")

                if not (flags & 0x4000) and (flags & 0x80):
                    dyn += 1
                if flags & 0x2000:
                    dj32 += 1
                if flags & 0x80:
                    elf += 1

                offs = int.from_bytes(mv[0x3c:0x40], byteorder="little")
                coffset = offs
                mzhdr_buf[:0x40] = buf
                can_copy_ovl += 1

                if rmstub or strip:
                    rmoverlay += 1

                nmoff = int.from_bytes(mv[0x28:0x2c], byteorder="little")
                if nmoff:
                    tp = (ObjType.OT_DJ32 if dj32 else ObjType.OT_E64).value
                    noff = find_offs(tp, buf)
                    if noff != -1:
                        ifile.seek(offs + noff + nmoff, os.SEEK_SET)
                        name_buf = ifile.read(20)
                        if b'\x00' in name_buf:
                            name = name_buf.split(b'\x00', 1)[0]
                        else:
                            name = name_buf
                        ifile.seek(offs, os.SEEK_SET)

                if info:
                    ibuf.append("dj64 file format\n")
                    if dyn:
                        ibuf.append("DOS payload dynamic\n")

                if info or rmoverlay:
                    type_map: int = int.from_bytes(mv[0x36:0x38], byteorder="little")
                    type_val: int = 0
                    sz: int = 0
                    i: int = 0

                    if not type_map:
                        sz = int.from_bytes(mv[0x1c:0x20], byteorder="little")
                        iprintf("Overlay 0 (i386/%s)\n\tat %i, size %i\n",
                                payload_dsc[ObjType.OT_COFF.value], offs, sz)
                    else:
                        while type_map > 0:
                            type_val = type_map & 0xf
                            prname = (type_val == ObjType.OT_D64.value or type_val == ObjType.OT_D32.value)
                            sz = int.from_bytes(mv[0x1c + i * 4:0x1c + i * 4 + 4], byteorder="little")
                            if not sz:
                                break
                            if info:
                                name_str = name.decode('utf-8', errors='ignore')
                                iprintf("Overlay %i (%s%s%s)\n\tat %i, size %i\n", i,
                                        identify(type_val, ifile, offs),
                                        " for " if prname else "", name_str if prname else "",
                                        offs, sz)
                            offs += sz
                            type_map >>= 4
                            i += 1

                    if rmstub:
                        tp = ((ObjType.OT_DJ32 if dj32 else ObjType.OT_E64) if elf else ObjType.OT_COFF).value
                        off = find_offs(tp, buf)
                        sz = find_size(tp, buf)
                        if off == -1 or sz == -1:
                            print(f"unable to find ovl {tp}", file=sys.stderr)
                            ifile.close()
                            return -1
                        coffset += off
                        coff_file_size = sz
                        break
                    elif strip and (type_val == ObjType.OT_D64.value or type_val == ObjType.OT_D32.value):
                        coff_file_size = offs - sz - coffset
                        assert i > 1
                        write_u32(mzhdr_buf, 0x1c + (i - 1) * 4, 0)

                    iprintf("Stub version: %i\n", buf[0x3b])
                    iprintf("Stub flags: 0x%04x\n", flags)
            else:
                blocks = buf[4] + buf[5] * 256
                partial = buf[2] + buf[3] * 256
                if info:
                    ibuf.append("exe/djgpp file format\n")
                coffset += blocks * 512
                if partial:
                    coffset += partial - 512

        elif buf[0] == 0x4c and buf[1] == 0x01:
            if info and not stub_v:
                iprintf("COFF payload at %li\n", coffset)
            if stub_v and (flags & 0x80):
                print("Unexpected COFF payload, header is invalid", file=sys.stderr)
                ifile.close()
                return -1
            break

        elif buf[0] == 0x7f and buf[1] == 0x45 and buf[2] == 0x4c and buf[3] == 0x46:
            if info and not stub_v:
                iprintf("ELF payload for %s at %li\n", elf_id(ifile, coffset), coffset)
            if stub_v and not (flags & 0x80):
                print("Unexpected ELF payload, header is invalid", file=sys.stderr)
                ifile.close()
                return -1
            break
        else:
            print("Warning: input file is neither COFF nor stubbed COFF", file=sys.stderr)
            break

    if stub_v:
        stub_ver = stub_v

    if not coff_file_size:
        ifile.seek(0, os.SEEK_END)
        coff_file_size = ifile.tell() - coffset

    ifile.seek(coffset, os.SEEK_SET)

    if can_copy_ovl:
        _binary_stub_exe_start[0x1c:0x3c] = mzhdr_buf[0x1c:0x3c]
    else:
        write_u32(_binary_stub_exe_start, 0x1c, coff_file_size)

    _binary_stub_exe_start[0x3b] = stub_ver
    write_u32(_binary_stub_exe_start, 0x3c, _binary_stub_exe_size)

    if info:
        print("".join(ibuf), end="")
        ifile.close()
        return 0

    tmpl_path: str = ""
    if oname:
        try:
            ofile = open(oname, "wb")
        except IOError as e:
            print(f"open({oname}): {e.strerror}", file=sys.stderr)
            ifile.close()
            return -1
    else:
        fd, tmpl_path = tempfile.mkstemp(prefix="djstub_")
        ofile = os.fdopen(fd, "wb")

    if not rmstub:
        ofile.write(_binary_stub_exe_start)

    while coff_file_size > 0:
        chunk_to_read = min(4096, coff_file_size) if rmoverlay else 4096
        buf = ifile.read(chunk_to_read)
        if not buf:
            break

        if rmoverlay and len(buf) > coff_file_size:
            buf = buf[:coff_file_size]

        ofile.write(buf)
        if rmoverlay:
            coff_file_size -= len(buf)

    ifile.close()
    ofile.close()

    ret: int = 0
    if not oname:
        try:
            oname = shutil.move(tmpl_path, fname)
        except IOError:
            ret = -1

    return ret

def print_help():
    print("Usage: stubify [-v] [-l <overlay>] [-o <out_file>] <program>\n\n"
          "<program> may be COFF or stubbed .exe.\n\n"
          "Options:\n"
          "-h -> print this help\n"
          "-v -> print version\n"
          "-V -> request minimum stub version\n"
          "-d -> verbose messages for debugging\n"
          "-i -> display file info\n"
          "-s -> strip last overlay\n"
          "-r -> remove stub (and overlay, if any)\n"
          "-l <file_name> -> link in <file_name> file as an overlay\n"
          "-t <type> -> set type of next link segment\n"
          "-n <offs> -> write name offset into an overlay info\n"
          "-o <name> -> write output into <name>\n"
          "-f <flags> -> write <flags> into an overlay info\n"
          "-g -> generate a new file\n",
        file=sys.stderr)

def copy_file(ovl: str, ofile) -> int:
    try:
        with open(ovl, "rb") as dst:
            while rd := dst.read(4096):
                ofile.write(rd)
    except IOError:
        return -1
    return 0

def main():
    global verbose, rmstub, strip, stub_ver, _binary_stub_exe_start, _binary_stub_exe_size
    generate: int = 0
    oname: str = ""
    info: int = 0
    noverlay: int = 0
    nmoffs: int = 0
    stub_flags: int = 0
    type_map: int = 0
    type_val: int = 0
    type_shift: int = 0
    req_ver: int = 0

    try:
        opts, args = getopt.getopt(sys.argv[1:], "dhvV:irsgl:t:o:n:f:S:")
    except getopt.GetoptError as e:
        print(f"Unknown option error: {e}", file=sys.stderr)
        print_help()
        sys.exit(1)

    for opt, optarg in opts:
        match opt:
            case '-v':
                print(f"djstubify version 0.{version}")
                sys.exit(0)

            case '-V':
                try:
                    req_ver = int(optarg)
                except ValueError:
                    req_ver = 0

                if not req_ver:
                    print(f"bad -V value {optarg}", file=sys.stderr)
                    sys.exit(1)
                elif req_ver > stub_ver:
                    print(f"requested stub ver {req_ver} but supported only {stub_ver}", file=sys.stderr)
                    sys.exit(1)
                elif req_ver == stub_ver - 1:
                    print(f"requested stub ver {req_ver} but supported is {stub_ver}", file=sys.stderr)
                    stub_ver = req_ver
                elif req_ver < stub_ver - 1:
                    print(f"requested old stub ver {req_ver}, supported is {stub_ver}", file=sys.stderr)
                    sys.exit(1)

            case '-d':
                verbose = True

            case '-i':
                info = True

            case '-r':
                rmstub = True

            case '-s':
                strip = True

            case '-g':
                generate = True

            case '-l':
                if not type_val:
                    print("Error: missing type value", file=sys.stderr)
                    sys.exit(1)
                overlay.append(optarg)
                noverlay += 1
                type_shift += 4

            case '-t':
                try:
                    type_val = int(optarg)
                except ValueError:
                    type_val = 0
                if not type_val:
                    print(f"Error: wrong -t value {optarg}", file=sys.stderr)
                    sys.exit(1)
                type_map |= type_val << type_shift

            case '-n':
                nmoffs = int(optarg, 0)

            case '-f':
                stub_flags |= int(optarg, 0)

            case '-o':
                oname = optarg

            case '-h':
                print_help()
                sys.exit(0)

            case '-S':
                try:
                    with open(optarg, "rb") as f:
                        _binary_stub_exe_start = bytearray(f.read())
                        _binary_stub_exe_size = len(_binary_stub_exe_start)
                except IOError as e:
                    print(f"failed to open {optarg}: {e.strerror}", file=sys.stderr)
                    sys.exit(1)

            case _:
                print(f"Unknown option: {opt}", file=sys.stderr)
                print_help()
                sys.exit(1)

    if _binary_stub_exe_start is None:
        print("-S agrument missing", file=sys.stderr)
        sys.exit(1)

    if (len(_binary_stub_exe_start) < 0x40 or
            _binary_stub_exe_start[0] != ord('M') or _binary_stub_exe_start[1] != ord('Z') or
            _binary_stub_exe_start[8] != 4 or _binary_stub_exe_start[9] != 0):
        print("stub corrupted, bad build", file=sys.stderr)
        return 1

    if not req_ver and stub_ver >= 8 and noverlay:
        if stub_ver == 8:
            stub_ver = 7
            stub_flags &= ~0x80

    v_printf("stubify for dj64 executables, copyright (C) 2023 stsp")

    if generate:
        if not oname:
            print("djstubify: -o missing", file=sys.stderr)
            return 1

        if noverlay:
            for i in range(noverlay):
                try:
                    stat_size = os.path.getsize(overlay[i])
                    write_u32(_binary_stub_exe_start, 0x1c + i * 4, stat_size)
                except OSError as e:
                    print(f"failed to stat {overlay[i]}: {e.strerror}", file=sys.stderr)
                    return 1

        write_u32(_binary_stub_exe_start, 0x28, nmoffs)
        write_u16(_binary_stub_exe_start, 0x36, type_map)
        write_u16(_binary_stub_exe_start, 0x38, stub_flags)
        _binary_stub_exe_start[0x3b] = stub_ver
        write_u32(_binary_stub_exe_start, 0x3c, _binary_stub_exe_size)

        try:
            ofile = open(oname, "wb")
        except IOError as e:
            print(f"open({oname}): {e.strerror}", file=sys.stderr)
            return 1

        v_printf(f"stubify: generate {oname}")
        ofile.write(_binary_stub_exe_start)

        for i in range(noverlay):
            if copy_file(overlay[i], ofile) < 0:
                print("failed to copy overlays", file=sys.stderr)
                ofile.close()
                try:
                    os.unlink(oname)
                except OSError:
                    pass
                return 1

        ofile.close()
        return 0

    else:
        if len(args) < 1:
            print_help()
            return 1

        err: int = coff2exe(args[0], oname, info)
        if err:
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
