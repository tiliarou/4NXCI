#ifndef NXCI_NSP_H_
#define NXCI_NSP_H_

#include <inttypes.h>
#include "filepath.h"

typedef struct {
	char *type;
	char *id;
	uint64_t size;
	char *hash;		// SHA-256
	char keygeneration;
} cnmt_xml_content_t;

typedef struct {
	char *tid;
	char *filepath;
	cnmt_xml_content_t contents[4];
} cnmt_xml_t;

typedef struct {
	char *filepath;
	uint64_t filesize;
} nsp_create_info_t;

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
	nsp_file_entry_table_t file_entry_table[7];
	char string_table[0x118];
} nsp_header_t;

extern nsp_create_info_t nsp_create_info[7];
extern cnmt_xml_t cnmt_xml;

void create_cnmt_xml();
void create_dummy_cert(filepath_t filepath);
void create_dummy_tik(filepath_t filepath);
void create_nsp();

#endif
