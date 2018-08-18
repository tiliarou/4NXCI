#ifndef NXCI_NSP_H_
#define NXCI_NSP_H_

#include <inttypes.h>
#include "filepath.h"

typedef struct {
    filepath_t filepath;
    char *nsp_filename;
    uint64_t filesize;
} nsp_entry_t;

typedef struct {
    filepath_t filepath;
    nsp_entry_t *nsp_entry;
} nsp_ctx_t;

typedef struct {
    uint64_t offset;
    uint64_t size;
    uint32_t filename_offset;
    uint32_t padding;
} nsp_file_entry_table_t;

typedef struct {
    char magic[4];
    uint32_t files_count;
    uint32_t string_table_size;
    uint32_t reserved;
    nsp_file_entry_table_t *file_entry_table;
    char *string_table;
} nsp_header_t;

void nsp_create(nsp_ctx_t *nsp_ctx, uint8_t entry_count);

extern nsp_ctx_t application_nsp;
extern nsp_ctx_t patch_nsp;
extern nsp_ctx_t addon_nsp;

#endif
