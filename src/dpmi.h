#ifndef DPMI_API_H
#define DPMI_API_H

#include <stdint.h>

typedef struct {
  unsigned short offset16;
  unsigned short segment;
} __dpmi_raddr;

typedef struct {
  uint32_t  offset32;
  unsigned short selector;
} __dpmi_paddr;

void	dpmi_yield(void);									/* INT 0x2F AX=1680 */

int	dpmi_allocate_ldt_descriptors(int _count);						/* DPMI 0.9 AX=0000 */
int	dpmi_free_ldt_descriptor(int _descriptor);						/* DPMI 0.9 AX=0001 */
int	dpmi_segment_to_descriptor(int _segment);						/* DPMI 0.9 AX=0002 */
int	dpmi_get_selector_increment_value(void);						/* DPMI 0.9 AX=0003 */
int	dpmi_get_segment_base_address(int _selector, uint32_t *_addr);			/* DPMI 0.9 AX=0006 */
int	dpmi_set_segment_base_address(int _selector, uint32_t _address);			/* DPMI 0.9 AX=0007 */
uint32_t dpmi_get_segment_limit(int _selector);						/* LSL instruction  */
int	dpmi_set_segment_limit(int _selector, uint32_t _limit);				/* DPMI 0.9 AX=0008 */
int	dpmi_get_descriptor_access_rights(int _selector);					/* LAR instruction  */
int	dpmi_set_descriptor_access_rights(int _selector, int _rights);			/* DPMI 0.9 AX=0009 */
int	dpmi_create_alias_descriptor(int _selector);						/* DPMI 0.9 AX=000a */
int	dpmi_get_descriptor(int _selector, void *_buffer);					/* DPMI 0.9 AX=000b */
int	dpmi_set_descriptor(int _selector, void *_buffer);					/* DPMI 0.9 AX=000c */
int	dpmi_allocate_specific_ldt_descriptor(int _selector);					/* DPMI 0.9 AX=000d */

int	dpmi_get_multiple_descriptors(int _count, void *_buffer);				/* DPMI 1.0 AX=000e */
int	dpmi_set_multiple_descriptors(int _count, void *_buffer);				/* DPMI 1.0 AX=000f */

int	dpmi_allocate_dos_memory(int _paragraphs, uint16_t *_ret_selector_or_max);			/* DPMI 0.9 AX=0100 */
int	dpmi_free_dos_memory(int _selector);							/* DPMI 0.9 AX=0101 */
int	dpmi_resize_dos_memory(int _selector, int _newpara, int *_ret_max);			/* DPMI 0.9 AX=0102 */

int	dpmi_get_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address);		/* DPMI 0.9 AX=0200 */
int	dpmi_set_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address);		/* DPMI 0.9 AX=0201 */
int	dpmi_get_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address);	/* DPMI 0.9 AX=0202 */
int	dpmi_set_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address);	/* DPMI 0.9 AX=0203 */
int	dpmi_get_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address);	/* DPMI 0.9 AX=0204 */
int	dpmi_set_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address);	/* DPMI 0.9 AX=0205 */

int	dpmi_get_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address);	/* DPMI 1.0 AX=0210 */
int	dpmi_get_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address);	/* DPMI 1.0 AX=0211 */
int	dpmi_set_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address);	/* DPMI 1.0 AX=0212 */
int	dpmi_set_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address);	/* DPMI 1.0 AX=0213 */

int	dpmi_get_state_save_restore_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm);		/* DPMI 0.9 AX=0305 */
int	dpmi_get_raw_mode_switch_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm);			/* DPMI 0.9 AX=0306 */

//int	dpmi_get_version(__dpmi_version_ret *_ret);						/* DPMI 0.9 AX=0400 */

int	dpmi_get_capabilities(int *_flags, char *vendor_info);				/* DPMI 1.0 AX=0401 */

//int	dpmi_get_free_memory_information(__dpmi_free_mem_info *_info);			/* DPMI 0.9 AX=0500 */
int	dpmi_allocate_memory(uint32_t size, uint32_t *addr, uint32_t *handle);
int	dpmi_free_memory(uint32_t _handle);						/* DPMI 0.9 AX=0502 */
int dpmi_resize_memory(uint32_t size, uint32_t handle, uint32_t *newaddr,
	uint32_t *newhandle);

int	dpmi_get_page_size(uint32_t *_size);						/* DPMI 0.9 AX=0604 */

/* These next four functions return the old state */
int	dpmi_get_and_disable_virtual_interrupt_state(void);					/* DPMI 0.9 AX=0900 */
int	dpmi_get_and_enable_virtual_interrupt_state(void);					/* DPMI 0.9 AX=0901 */
int	dpmi_get_and_set_virtual_interrupt_state(void);				/* DPMI 0.9 AH=09   */
int	dpmi_get_virtual_interrupt_state(void);						/* DPMI 0.9 AX=0902 */

int	dpmi_get_vendor_specific_api_entry_point(char *_id, __dpmi_paddr *_api);		/* DPMI 0.9 AX=0a00 */

#endif
