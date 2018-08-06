#ifndef NXCI_CNMT_H_
#define NXCI_CNMT_H_

#include <inttypes.h>
#include "pfs0.h"

typedef struct {
	char *type;
	char id[0x20];
	uint64_t size;
	char *hash;		// SHA-256
	unsigned char keygeneration;
} cnmt_xml_content_t;

typedef struct {
	char *tid;
	char *filepath;
	cnmt_xml_content_t contents[4];
} cnmt_xml_t;

typedef struct {
	unsigned char hash[0x20];
	uint8_t padding[0x1E0];
} pfs0_hashtable_t;

typedef struct {
	uint64_t tid;
	uint32_t version;
	uint8_t type;
	uint8_t unknown1;
	uint16_t offset;
	uint16_t content_count;
	uint16_t meta_count;
	uint8_t unknown2[0xC];
	uint64_t patchid;
	uint64_t sysversion;
} application_cnmt_header_t;

typedef struct {
	unsigned char hash[0x20];
	uint8_t ncaid[0x10];
	uint8_t size[0x06];
	uint8_t type;
	uint8_t padding;
} application_cnmt_content_t;


typedef struct {
	pfs0_hashtable_t hashtable;
	pfs0_header_t header;
    pfs0_file_entry_t file_entry;
    char string_table[0x38];
	application_cnmt_header_t application_cnmt_header;
	application_cnmt_content_t application_cnmt_contents[3];
	unsigned char digest[0x20];
	uint8_t padding[0xA8];
} pfs0_t;

extern cnmt_xml_t cnmt_xml;
extern application_cnmt_content_t application_cnmt_contents[3];

#endif
