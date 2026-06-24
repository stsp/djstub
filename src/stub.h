#ifndef STUB_H
#define STUB_H

#include "util.h"

struct ldops {
    void *(*read_headers)(int ifile);
    uint32_t (*get_va)(void *handle);
    uint32_t (*get_length)(void *handle);
    uint32_t (*get_entry)(void *handle);
    void (*read_sections)(void *handle, char_far ptr, int ifile,
            uint32_t offset);
};

#endif
