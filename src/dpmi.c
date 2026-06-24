/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "dpmi.h"

void dpmi_yield(void)
{									/* INT 0x2F AX=1680 */
    asm(
        "mov eax, 0x1680\n"
        "int 0x2f\n"
    );
}

int dpmi_allocate_ldt_descriptors(int _count)
{						/* DPMI 0.9 AX=0000 */
    asm(
        "mov eax, 0\n"
        "mov ecx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_free_ldt_descriptor(int _descriptor)
{						/* DPMI 0.9 AX=0001 */
    asm(
        "mov eax, 1\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_segment_to_descriptor(int _segment)
{						/* DPMI 0.9 AX=0002 */
    asm(
        "mov eax, 2\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_selector_increment_value(void)
{						/* DPMI 0.9 AX=0003 */
    asm(
        "mov eax, 3\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_segment_base_address(int _selector, uint32_t *_addr)
{			/* DPMI 0.9 AX=0006 */
    asm(
        "mov eax, 6\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edi, [ebp + 12]\n"
        "mov [edi], dx\n"
        "mov [edi + 2], cx\n"
    );
}

int dpmi_set_segment_base_address(int _selector, uint32_t _address)
{			/* DPMI 0.9 AX=0007 */
    asm(
        "mov eax, 7\n"
        "mov ebx, [ebp + 8]\n"
        "movzx ecx, word [ebp + 12 + 2]\n"
        "movzx edx, word [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

uint32_t dpmi_get_segment_limit(int _selector)
{						/* LSL instruction  */
    asm("lsl eax, [ebp + 8]\n");
}

int dpmi_set_segment_limit(int _selector, uint32_t _limit)
{				/* DPMI 0.9 AX=0008 */
    asm(
        "mov eax, 8\n"
        "mov ebx, [ebp + 8]\n"
        "movzx ecx, word [ebp + 12 + 2]\n"
        "movzx edx, word [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_descriptor_access_rights(int _selector)
{					/* LAR instruction  */
    asm("lar eax, [ebp + 8]\n");
}

int dpmi_set_descriptor_access_rights(int _selector, int _rights)
{			/* DPMI 0.9 AX=0009 */
    asm(
        "mov eax, 9\n"
        "mov ebx, [ebp + 8]\n"
        "mov ecx, [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_create_alias_descriptor(int _selector)
{						/* DPMI 0.9 AX=000a */
    asm(
        "mov eax, 0xa\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_descriptor(int _selector, void *_buffer)
{					/* DPMI 0.9 AX=000b */
    asm(
        "mov eax, 0xb\n"
        "mov ebx, [ebp + 8]\n"
        "mov edi, [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_set_descriptor(int _selector, void *_buffer)
{					/* DPMI 0.9 AX=000c */
    asm(
        "mov eax, 0xc\n"
        "mov ebx, [ebp + 8]\n"
        "mov edi, [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_allocate_specific_ldt_descriptor(int _selector)
{					/* DPMI 0.9 AX=000d */
    asm(
        "mov eax, 0xd\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_multiple_descriptors(int _count, void *_buffer)
{				/* DPMI 1.0 AX=000e */
    asm(
        "mov eax, 0xe\n"
        "mov ecx, [ebp + 8]\n"
        "mov edi, [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_set_multiple_descriptors(int _count, void *_buffer)
{				/* DPMI 1.0 AX=000f */
    asm(
        "mov eax, 0xf\n"
        "mov ecx, [ebp + 8]\n"
        "mov edi, [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_allocate_dos_memory(int _paragraphs, uint16_t *_ret_selector_or_max)
{			/* DPMI 0.9 AX=0100 */
    asm(
        "mov eax, 0x100\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "mov edi, [ebp + 12]\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        "mov [edi], ebx\n"
        "jmp .done\n"
        ".ok:\n"
        "mov [edi], edx\n"
        ".done:\n"
    );
}

int dpmi_free_dos_memory(int _selector)
{							/* DPMI 0.9 AX=0101 */
    asm(
        "mov eax, 0x101\n"
        "mov edx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_resize_dos_memory(int _selector, int _newpara, int *_ret_max)
{			/* DPMI 0.9 AX=0102 */
    asm(
        "mov eax, 0x102\n"
        "mov ebx, [ebp + 12]\n"
        "mov edx, [ebp + 8]\n"
        "int 0x31\n"
        "mov edi, [ebp + 16]\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        "mov [edi], ebx\n"
        ".ok:\n"
    );
}

int dpmi_get_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address)
{		/* DPMI 0.9 AX=0200 */
    asm(
        "mov eax, 0x200\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "mov edi, [ebp + 12]\n"
        "mov [edi], dx\n"
        "mov [edi + 2], cx\n"
    );
    return 0;  // no error
}

int dpmi_set_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address)
{		/* DPMI 0.9 AX=0201 */
    asm(
        "mov eax, 0x201\n"
        "mov ebx, [ebp + 8]\n"
        "mov esi, [ebp + 12]\n"
        "movzx ecx, word [esi + 2]\n"
        "movzx edx, word [esi]\n"
        "int 0x31\n"
    );
    return 0;  // no error
}

int dpmi_get_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address)
{	/* DPMI 0.9 AX=0202 */
    asm(
        "mov eax, 0x202\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edi, [ebp + 12]\n"
        "mov [edi], edx\n"
        "mov [edi + 4], cx\n"
    );
}

int dpmi_set_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address)
{	/* DPMI 0.9 AX=0203 */
    asm(
        "mov eax, 0x203\n"
        "mov ebx, [ebp + 8]\n"
        "mov esi, [ebp + 12]\n"
        "movzx ecx, word [esi + 4]\n"
        "mov edx, [esi]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address)
{	/* DPMI 0.9 AX=0204 */
    asm(
        "mov eax, 0x204\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "mov edi, [ebp + 12]\n"
        "mov [edi], edx\n"
        "mov [edi + 4], cx\n"
    );
    return 0;
}

int dpmi_set_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address)
{	/* DPMI 0.9 AX=0205 */
    asm(
        "mov eax, 0x205\n"
        "mov ebx, [ebp + 8]\n"
        "mov esi, [ebp + 12]\n"
        "movzx ecx, word [esi + 4]\n"
        "mov edx, [esi]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address)
{	/* DPMI 1.0 AX=0210 */
    asm(
        "mov eax, 0x210\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edi, [ebp + 12]\n"
        "mov [edi], edx\n"
        "mov [edi + 4], cx\n"
    );
}

int dpmi_get_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address)
{	/* DPMI 1.0 AX=0211 */
    asm(
        "mov eax, 0x211\n"
        "mov ebx, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edi, [ebp + 12]\n"
        "mov [edi], edx\n"
        "mov [edi + 4], cx\n"
    );
}

int dpmi_set_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address)
{	/* DPMI 1.0 AX=0212 */
    asm(
        "mov eax, 0x212\n"
        "mov ebx, [ebp + 8]\n"
        "mov esi, [ebp + 12]\n"
        "movzx ecx, word [esi + 4]\n"
        "mov edx, [esi]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_set_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address)
{	/* DPMI 1.0 AX=0213 */
    asm(
        "mov eax, 0x213\n"
        "mov ebx, [ebp + 8]\n"
        "mov esi, [ebp + 12]\n"
        "movzx ecx, word [esi + 4]\n"
        "mov edx, [esi]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

#if 0
int dpmi_allocate_real_mode_callback(void (*_handler)(void), __dpmi_regs *__regs, __dpmi_raddr *_ret)
{ /* DPMI 0.9 AX=0303 */
    return 0;
}

int dpmi_free_real_mode_callback(__dpmi_raddr *_addr)
{					/* DPMI 0.9 AX=0304 */
    return 0;
}

int dpmi_get_state_save_restore_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm)
{		/* DPMI 0.9 AX=0305 */
    return 0;
}

int dpmi_get_raw_mode_switch_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm)
{			/* DPMI 0.9 AX=0306 */
    return 0;
}

int dpmi_get_version(__dpmi_version_ret *_ret)
{						/* DPMI 0.9 AX=0400 */
    return 0;
}

int dpmi_get_capabilities(int *_flags, char *vendor_info)
{				/* DPMI 1.0 AX=0401 */
    return 0;
}

int dpmi_get_free_memory_information(__dpmi_free_mem_info *_info)
{			/* DPMI 0.9 AX=0500 */
    return 0;
}
#endif

int dpmi_allocate_memory(uint32_t size, uint32_t *addr, uint32_t *handle)
{						/* DPMI 0.9 AX=0501 */
    asm(
        "mov eax, 0x501\n"
        "movzx ebx, word [ebp + 8 + 2]\n"
        "movzx ecx, word [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edx, [ebp + 12]\n"
        "mov [edx + 2], bx\n"
        "mov [edx], cx\n"
        "mov edx, [ebp + 16]\n"
        "mov [edx + 2], si\n"
        "mov [edx], di\n"
    );
}

int dpmi_free_memory(uint32_t _handle)
{						/* DPMI 0.9 AX=0502 */
    asm(
        "mov eax, 0x502\n"
        "movzx esi, word [ebp + 8 + 2]\n"
        "movzx edi, word [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_resize_memory(uint32_t size, uint32_t handle, uint32_t *newaddr,
    uint32_t *newhandle)
{						/* DPMI 0.9 AX=0503 */
    asm(
        "mov eax, 0x503\n"
        "movzx ebx, word [ebp + 8 + 2]\n"
        "movzx ecx, word [ebp + 8]\n"
        "movzx esi, word [ebp + 12 + 2]\n"
        "movzx edi, word [ebp + 12]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edx, [ebp + 16]\n"
        "mov [edx + 2], bx\n"
        "mov [edx], cx\n"
        "mov edx, [ebp + 20]\n"
        "mov [edx + 2], si\n"
        "mov [edx], di\n"
    );
}

#if 0
int dpmi_allocate_linear_memory(__dpmi_meminfo *_info, int _commit)
{			/* DPMI 1.0 AX=0504 */
    return 0;
}

int dpmi_resize_linear_memory(__dpmi_meminfo *_info, int _commit)
{			/* DPMI 1.0 AX=0505 */
    return 0;
}

int dpmi_get_page_attributes(__dpmi_meminfo *_info, short *_buffer)
{			/* DPMI 1.0 AX=0506 */
    return 0;
}

int dpmi_set_page_attributes(__dpmi_meminfo *_info, short *_buffer)
{			/* DPMI 1.0 AX=0507 */
    return 0;
}

int dpmi_map_device_in_memory_block(__dpmi_meminfo *_info, uint32_t _physaddr)
{	/* DPMI 1.0 AX=0508 */
    return 0;
}

int dpmi_map_conventional_memory_in_memory_block(__dpmi_meminfo *_info, uint32_t _physaddr)
{ /* DPMI 1.0 AX=0509 */
    return 0;
}

int dpmi_get_memory_block_size_and_base(__dpmi_meminfo *_info)
{				/* DPMI 1.0 AX=050a */
    return 0;
}

int dpmi_get_memory_information(__dpmi_memory_info *_buffer)
{				/* DPMI 1.0 AX=050b */
    return 0;
}

int dpmi_lock_linear_region(__dpmi_meminfo *_info)
{					/* DPMI 0.9 AX=0600 */
    return 0;
}

int dpmi_unlock_linear_region(__dpmi_meminfo *_info)
{					/* DPMI 0.9 AX=0601 */
    return 0;
}

int dpmi_mark_real_mode_region_as_pageable(__dpmi_meminfo *_info)
{			/* DPMI 0.9 AX=0602 */
    return 0;
}

int dpmi_relock_real_mode_region(__dpmi_meminfo *_info)
{					/* DPMI 0.9 AX=0603 */
    return 0;
}
#endif

int dpmi_get_page_size(uint32_t *_size)
{						/* DPMI 0.9 AX=0604 */
    asm(
        "mov eax, 0x604\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edi, [ebp + 8]\n"
        "mov [edi + 2], bx\n"
        "mov [edi], cx\n"
    );
}

#if 0
int dpmi_mark_page_as_demand_paging_candidate(__dpmi_meminfo *_info)
{			/* DPMI 0.9 AX=0702 */
    return 0;
}

int dpmi_discard_page_contents(__dpmi_meminfo *_info)
{					/* DPMI 0.9 AX=0703 */
    return 0;
}

int dpmi_physical_address_mapping(__dpmi_meminfo *_info)
{					/* DPMI 0.9 AX=0800 */
    return 0;
}

int dpmi_free_physical_address_mapping(__dpmi_meminfo *_info)
{				/* DPMI 0.9 AX=0801 */
    return 0;
}
#endif

/* These next four functions return the old state */
int dpmi_get_and_disable_virtual_interrupt_state(void)
{					/* DPMI 0.9 AX=0900 */
    asm(
        "mov eax, 0x900\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_and_enable_virtual_interrupt_state(void)
{					/* DPMI 0.9 AX=0901 */
    asm(
        "mov eax, 0x901\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_virtual_interrupt_state(void)
{						/* DPMI 0.9 AX=0902 */
    asm(
        "mov eax, 0x902\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
    );
}

int dpmi_get_vendor_specific_api_entry_point(char *_id, __dpmi_paddr *_api)
{		/* DPMI 0.9 AX=0a00 */
    asm(
        "push es\n"
        "mov eax, 0xa00\n"
        "mov esi, [ebp + 8]\n"
        "int 0x31\n"
        "jnc .ok\n"
        "mov eax, -1\n"
        ".ok:\n"
        "mov edx, [ebp + 12]\n"
        "mov [edx], edi\n"
        "mov [edx + 4], es\n"
        "pop es\n"
    );
}

#if 0
int dpmi_set_debug_watchpoint(__dpmi_meminfo *_info, int _type)
{				/* DPMI 0.9 AX=0b00 */
    return 0;
}

int dpmi_clear_debug_watchpoint(uint32_t _handle)
{					/* DPMI 0.9 AX=0b01 */
    return 0;
}

int dpmi_get_state_of_debug_watchpoint(uint32_t _handle, int *_status)
{		/* DPMI 0.9 AX=0b02 */
    return 0;
}

int dpmi_reset_debug_watchpoint(uint32_t _handle)
{					/* DPMI 0.9 AX=0b03 */
    return 0;
}

int dpmi_install_resident_service_provider_callback(__dpmi_callback_info *_info)
{		/* DPMI 1.0 AX=0c00 */
    return 0;
}

int dpmi_terminate_and_stay_resident(int return_code, int paragraphs_to_keep)
{		/* DPMI 1.0 AX=0c01 */
    return 0;
}

int dpmi_allocate_shared_memory(__dpmi_shminfo *_info)
{					/* DPMI 1.0 AX=0d00 */
    return 0;
}

int dpmi_free_shared_memory(uint32_t _handle)
{					/* DPMI 1.0 AX=0d01 */
    return 0;
}

int dpmi_serialize_on_shared_memory(uint32_t _handle, int _flags)
{			/* DPMI 1.0 AX=0d02 */
    return 0;
}

int dpmi_free_serialization_on_shared_memory(uint32_t _handle, int _flags)
{		/* DPMI 1.0 AX=0d03 */
    return 0;
}

int dpmi_get_coprocessor_status(void)
{							/* DPMI 1.0 AX=0e00 */
    return 0;
}

int dpmi_set_coprocessor_emulation(int _flags)
{						/* DPMI 1.0 AX=0e01 */
    return 0;
}
#endif
