#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libgen.h>
#include "nsp.h"
#include "dummy_files.h"
#include "cnmt.h"
#include "utils.h"

/* Create .cnmt.xml
 The process is done without xml libs cause i don't want to add more dependency for now
  */
void create_cnmt_xml()
{
	printf("Creating .cnmt.xml %s\n", cnmt_xml.filepath);
	FILE *file = fopen(cnmt_xml.filepath,"wb");
	fprintf(file,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\x0D\x0A");
	fprintf(file,"<ContentMeta>\x0D\x0A");
	fprintf(file,"  <Type>Application</Type>\x0D\x0A");
	fprintf(file,"  <Id>0x%s</Id>\x0D\x0A",cnmt_xml.tid);
	fprintf(file,"  <Version>0</Version>\x0D\x0A");
	fprintf(file,"  <RequiredDownloadSystemVersion>0</RequiredDownloadSystemVersion>\x0D\x0A");
	for (int index=0;index<4;index++) {
		fprintf(file,"  <Content>\x0D\x0A");
		fprintf(file,"    <Type>%s</Type>\x0D\x0A", cnmt_xml.contents[index].type);
		fprintf(file,"    <Id>%s</Id>\x0D\x0A", cnmt_xml.contents[index].id);
		fprintf(file,"    <Size>%" PRIu64 "</Size>\x0D\x0A", cnmt_xml.contents[index].size);
		fprintf(file,"    <Hash>%s</Hash>\x0D\x0A", cnmt_xml.contents[index].hash);
		fprintf(file,"    <KeyGeneration>%u</KeyGeneration>\x0D\x0A", cnmt_xml.contents[index].keygeneration);
		fprintf(file,"  </Content>\x0D\x0A");
	}
	// Hardcode Digest, it's an unknown value
	fprintf(file,"  <Digest>0000000000000000000000000000000000000000000000000000000000000000</Digest>\x0D\x0A");
	fprintf(file,"  <KeyGenerationMin>%u</KeyGenerationMin>\x0D\x0A",cnmt_xml.contents[3].keygeneration);
	// Setting RequiredSystemVersion  to 1.0.0 firmware
	fprintf(file,"  <RequiredSystemVersion>0</RequiredSystemVersion>\x0D\x0A");
	// PatchId is always equals to first 13 chars of title id + 800
	fprintf(file,"  <PatchId>0x%.*s800</PatchId>\x0D\x0A",13,cnmt_xml.tid);
	fprintf(file,"</ContentMeta>");

	// Set file size for creating nsp
	nsp_create_info[4].filesize = (uint64_t)ftello64(file);
	// Set file path for creating nsp
	nsp_create_info[4].filepath = cnmt_xml.filepath;
	nsp_create_info[4].nsp_filename = (char*)calloc(1,42);
	// Set new filename for creating nsp
	strcpy(nsp_create_info[4].nsp_filename,cnmt_xml.contents[3].id);
	strcat(nsp_create_info[4].nsp_filename,".cnmt.xml");
	fclose(file);

}

void create_dummy_cert(filepath_t filepath)
{
	filepath_t dummy_cert_path;
	filepath_copy(&dummy_cert_path,&filepath);
	// cert filename is: title id (16 bytes) + key generation (16 bytes) + .cert
	filepath_append(&dummy_cert_path,"%s000000000000000%u.cert",cnmt_xml.tid,cnmt_xml.contents[3].keygeneration);
	printf("Creating dummy cert %s\n",dummy_cert_path.char_path);
	FILE *file;
	if (!(file = fopen(dummy_cert_path.char_path, "wb"))) {
    	fprintf(stderr,"unable to create dummy cert\n");
    	exit(EXIT_FAILURE);
	}
	fwrite(dummy_cert,1,DUMMYCERTSIZE,file);
	fclose(file);

	// Set file size for creating nsp
	nsp_create_info[5].filesize = DUMMYCERTSIZE;
	// Set file name for creating nsp
	nsp_create_info[5].filepath = (char*)calloc(1,strlen(dummy_cert_path.char_path)+1);
	strcpy(nsp_create_info[5].filepath,dummy_cert_path.char_path);
	// Set new filename for creating nsp
	nsp_create_info[5].nsp_filename = basename(nsp_create_info[5].filepath);
}

void create_dummy_tik(filepath_t filepath)
{
	filepath_t dummy_tik_path;
	filepath_copy(&dummy_tik_path,&filepath);
	// tik filename is: title id (16 bytes) + key generation (16 bytes) + .tik
	filepath_append(&dummy_tik_path,"%s000000000000000%u.tik",cnmt_xml.tid,cnmt_xml.contents[3].keygeneration);
	printf("Creating dummy tik %s\n\n",dummy_tik_path.char_path);
	FILE *file;
	if (!(file = fopen(dummy_tik_path.char_path, "wb"))) {
		fprintf(stderr,"unable to create dummy tik\n");
		exit(EXIT_FAILURE);
	}
	fwrite(dummy_tik,1,DUMMYTIKSIZE,file);
	fclose(file);

	// Set file size for creating nsp
	nsp_create_info[6].filesize = DUMMYTIKSIZE;
	// Set file path for creating nsp
	nsp_create_info[6].filepath = (char*)calloc(1,strlen(dummy_tik_path.char_path) + 1);
	strcpy(nsp_create_info[6].filepath,dummy_tik_path.char_path);
	// Set new filename for creating nsp
	nsp_create_info[6].nsp_filename = basename(nsp_create_info[6].filepath);
}

void create_nsp()
{
	// nsp file name is tid.nsp
	char *nsp_path = (char*)calloc(1,21);
	strcpy(nsp_path,cnmt_xml.tid);
	strcat(nsp_path,".nsp");
	printf("Creating nsp %s\n",nsp_path);
	nsp_header_t nsp_header = { .magic = {0x50 , 0x46, 0x53,  0x30}, // PFS0
								.files_count = 7, // Always 7 files
								.string_table_size = 280, 	// Always 280 bytes
								.string_table = {0},
								.reserved = 0,
	};

	uint64_t offset = 0;
	uint32_t filename_offset = 0;

	for (int index=0;index<7;index++) {
		nsp_header.file_entry_table[index].offset = offset;
		nsp_header.file_entry_table[index].filename_offset = filename_offset;
		nsp_header.file_entry_table[index].padding = 0;
		nsp_header.file_entry_table[index].size = nsp_create_info[index].filesize;
		offset += nsp_create_info[index].filesize;
		strcpy(nsp_header.string_table + filename_offset,nsp_create_info[index].nsp_filename);
		filename_offset += strlen(nsp_create_info[index].nsp_filename) + 1;
	}

	FILE *nsp_file;
	if ((nsp_file = fopen(nsp_path, "wb")) == NULL) {
    	fprintf(stderr,"unable to create nsp\n");
    	exit(EXIT_FAILURE);
	}
	fwrite(&nsp_header,sizeof(nsp_header),1,nsp_file);

	for (int index2=0;index2<7;index2++) {
		FILE *nsp_data_file = fopen(nsp_create_info[index2].filepath, "rb");
		printf("Packing %s into %s\n",nsp_create_info[index2].filepath, nsp_path);

			if (nsp_data_file == NULL) {
			    fprintf(stderr, "Failed to open %s!\n", nsp_create_info[index2].filepath);
			    exit(EXIT_FAILURE);
			}
		    uint64_t read_size = 0x4000000; // 4 MB buffer.
			unsigned char *buf = malloc(read_size);
			if (buf == NULL) {
			    fprintf(stderr, "Failed to allocate file-read buffer!\n");
			    exit(EXIT_FAILURE);
			}

			uint64_t ofs = 0;
			while (ofs < nsp_create_info[index2].filesize) {
			    if (ofs + read_size >= nsp_create_info[index2].filesize) read_size = nsp_create_info[index2].filesize - ofs;
			    if (fread(buf, 1, read_size, nsp_data_file) != read_size) {
			        fprintf(stderr, "Failed to read file %s\n",nsp_create_info[index2].filepath);
			        exit(EXIT_FAILURE);
			    }
			    fwrite(buf,read_size,1,nsp_file);
			    ofs += read_size;
			}
		fclose(nsp_data_file);
	}

	fclose(nsp_file);
	free(nsp_path);
	printf("\n");
}
