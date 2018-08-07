#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <inttypes.h>
#include "nca.h"
#include "aes.h"
#include "pki.h"
#include "sha.h"
#include "rsa.h"
#include "utils.h"
#include "extkeys.h"
#include "filepath.h"
#include "nsp.h"

/* Initialize the context. */
void nca_init(nca_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));
}

void nca_free_section_contexts(nca_ctx_t *ctx) {
    for (unsigned int i = 0; i < 4; i++) {
        if (ctx->section_contexts[i].is_present) {
            if (ctx->section_contexts[i].aes) {
                free_aes_ctx(ctx->section_contexts[i].aes);
            }
            if (ctx->section_contexts[i].type == PFS0 && ctx->section_contexts[i].pfs0_ctx.is_exefs) {
                free(ctx->section_contexts[i].pfs0_ctx.npdm);
            } else if (ctx->section_contexts[i].type == ROMFS) {
                if (ctx->section_contexts[i].romfs_ctx.directories) {
                    free(ctx->section_contexts[i].romfs_ctx.directories);
                }
                if (ctx->section_contexts[i].romfs_ctx.files) {
                    free(ctx->section_contexts[i].romfs_ctx.files);
                }
            }  else if (ctx->section_contexts[i].type == NCA0_ROMFS) {
                if (ctx->section_contexts[i].nca0_romfs_ctx.directories) {
                    free(ctx->section_contexts[i].nca0_romfs_ctx.directories);
                }
                if (ctx->section_contexts[i].nca0_romfs_ctx.files) {
                    free(ctx->section_contexts[i].nca0_romfs_ctx.files);
                }
            } else if (ctx->section_contexts[i].type == BKTR) {
                if (ctx->section_contexts[i].bktr_ctx.subsection_block) {
                    free(ctx->section_contexts[i].bktr_ctx.subsection_block);
                }
                if (ctx->section_contexts[i].bktr_ctx.relocation_block) {
                    free(ctx->section_contexts[i].bktr_ctx.relocation_block);
                }
                if (ctx->section_contexts[i].bktr_ctx.directories) {
                    free(ctx->section_contexts[i].bktr_ctx.directories);
                }
                if (ctx->section_contexts[i].bktr_ctx.files) {
                    free(ctx->section_contexts[i].bktr_ctx.files);
                }
            }
        }
    }
}

/* Updates the CTR for an offset. */
void nca_update_ctr(unsigned char *ctr, uint64_t ofs) {
    ofs >>= 4;
    for (unsigned int j = 0; j < 0x8; j++) {
        ctr[0x10-j-1] = (unsigned char)(ofs & 0xFF);
        ofs >>= 8;
    }
}

static void nca_save(nca_ctx_t *ctx) {
    /* Rewrite header */
	fseeko64(ctx->file, 0, SEEK_SET);
	if (!fwrite(&ctx->header, 1, 0xC00, ctx->file)) {
		fprintf(stderr,"Unable to patch NCA header");
		exit(EXIT_FAILURE);
	}
    fclose(ctx->file);
}

char *nca_get_content_type(nca_ctx_t *ctx) {
    switch (ctx->header.content_type) {
        case 0:
            return "Program";
            break;
        case 1:
            return "Meta";
            break;
        case 2:
            return "Control";
            break;
        case 3:
            return "LegalInformation";
            break;
        case 4:
            return "Data";
            break;
        default:
        	fprintf(stderr, "Unknown NCA content type");
        	exit(EXIT_FAILURE);
    }
}


// For writing xml tags in proper order
int nca_type_to_index(uint8_t nca_type)
{
	switch (nca_type) {
	case 0:
		return 0;
		break;
	case 1:
		return 3;
		break;
	case 2:
		return 1;
		break;
	case 3:
		return 2;
		break;
	default:
    	fprintf(stderr, "Unknown NCA content type");
    	exit(EXIT_FAILURE);
	}
}

int nca_type_to_cnmt_type(uint8_t nca_type)
{
	switch (nca_type) {
	case 0:
		return 1;
		break;
	case 1:
		return 3;
		break;
	case 2:
		return 5;
		break;
	default:
    	fprintf(stderr, "Unknown NCA content type");
    	exit(EXIT_FAILURE);
	}
}

// Heavily modify meta file
void meta_process(nca_ctx_t *ctx)
{
	// Set header and pfs0 superblock values for cnmt.nca
	ctx->header.nca_size = 0x1000;
	ctx->header.section_entries[0].media_start_offset = 0x06;
	ctx->header.section_entries[0].media_end_offset = 0x08;
	ctx->header.fs_headers[0].crypt_type = 0x03;
	ctx->header.fs_headers[0].pfs0_superblock.block_size = 0x1000;
	ctx->header.fs_headers[0].pfs0_superblock.hash_table_size = 0x20;
	ctx->header.fs_headers[0].pfs0_superblock.pfs0_offset = 0x200;
	ctx->header.fs_headers[0].pfs0_superblock.pfs0_size = 0x158;

	pfs0_t pfs0 = {
			.hashtable.padding = {0},
			.header.magic = 0x30534650,					// PFS0
			.header.num_files = 0x01,					// Application_tid.cnmt
			.header.string_table_size = 0x38,
			.header.reserved = 0,
			.file_entry.offset = 0,
			.file_entry.size = 0xF8,
			.file_entry.string_table_offset = 0,
			.file_entry.reserved = 0,
			.string_table = {'\0'},
			.application_cnmt_header.version = 0,
			.application_cnmt_header.type = 0x80,		//  Regular application
			.application_cnmt_header.offset = 0x10,
			.application_cnmt_header.content_count = 0x03,
			.application_cnmt_header.meta_count = 0,
			.application_cnmt_header.unknown2 = {0},
			.application_cnmt_header.unknown1 = 0,
			.application_cnmt_header.tid = ctx->header.title_id,
			.application_cnmt_header.patchid = ctx->header.title_id + 0x800,
			.application_cnmt_header.sysversion = 0,
			.digest = {0},
			.padding = {0}
	};

	// String table = Application_tid.cnmt
	strcat(pfs0.string_table,"Application_");
	strncat(pfs0.string_table,cnmt_xml.tid,16);
	strcat(pfs0.string_table,".cnmt");

	memcpy(pfs0.application_cnmt_contents,application_cnmt_contents,sizeof(application_cnmt_contents));

	// Calculate PFS0 hash
	sha_ctx_t *pfs0_sha_ctx = new_sha_ctx(HASH_TYPE_SHA256,0);
	unsigned char *pfs0_hash_result = (unsigned char*)calloc(1,33);
	sha_update(pfs0_sha_ctx,&pfs0.header,sizeof(pfs0.header));
	sha_update(pfs0_sha_ctx,&pfs0.file_entry,sizeof(pfs0.file_entry));
	sha_update(pfs0_sha_ctx,&pfs0.string_table,sizeof(pfs0.string_table));
	sha_update(pfs0_sha_ctx,&pfs0.application_cnmt_header,sizeof(pfs0.application_cnmt_header));
	sha_update(pfs0_sha_ctx,&pfs0.application_cnmt_contents,sizeof(pfs0.application_cnmt_contents));
	sha_update(pfs0_sha_ctx,&pfs0.digest,sizeof(pfs0.digest));
	sha_get_hash(pfs0_sha_ctx,pfs0_hash_result);
	memcpy(pfs0.hashtable.hash,pfs0_hash_result,32);

	meta_save(ctx, &pfs0);

	// Calculate PFS0 superblock hash
	sha_ctx_t *pfs0_superblock_sha_ctx = new_sha_ctx(HASH_TYPE_SHA256,0);
	unsigned char *pfs0_superblock_hash_result = (unsigned char*)calloc(1,33);
	sha_update(pfs0_superblock_sha_ctx,pfs0_hash_result,32);
	sha_get_hash(pfs0_superblock_sha_ctx,pfs0_superblock_hash_result);
	memcpy(ctx->header.fs_headers[0].pfs0_superblock.master_hash,pfs0_superblock_hash_result,32);
	free(pfs0_superblock_hash_result);
	free(pfs0_hash_result);

	// Calculate section hash
	sha_ctx_t *pfs0_section_sha_ctx = new_sha_ctx(HASH_TYPE_SHA256,0);
	unsigned char *pfs0_section_hash_result = (unsigned char*)calloc(1,33);
	uint8_t pfs0_section_zeros[0x1B0] = { 0 };
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0]._0x0,sizeof(ctx->header.fs_headers[0]._0x0));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0]._0x1,sizeof(ctx->header.fs_headers[0]._0x1));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].partition_type,sizeof(ctx->header.fs_headers[0].partition_type));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].fs_type,sizeof(ctx->header.fs_headers[0].fs_type));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].crypt_type,sizeof(ctx->header.fs_headers[0].crypt_type));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0]._0x5,sizeof(ctx->header.fs_headers[0]._0x5));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].pfs0_superblock.master_hash,sizeof(ctx->header.fs_headers[0].pfs0_superblock.master_hash));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].pfs0_superblock.block_size,sizeof(ctx->header.fs_headers[0].pfs0_superblock.block_size));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].pfs0_superblock.always_2,sizeof(ctx->header.fs_headers[0].pfs0_superblock.always_2));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].pfs0_superblock.hash_table_offset,sizeof(ctx->header.fs_headers[0].pfs0_superblock.hash_table_offset));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].pfs0_superblock.hash_table_size,sizeof(ctx->header.fs_headers[0].pfs0_superblock.hash_table_size));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].pfs0_superblock.pfs0_offset,sizeof(ctx->header.fs_headers[0].pfs0_superblock.pfs0_offset));
	sha_update(pfs0_section_sha_ctx,&ctx->header.fs_headers[0].pfs0_superblock.pfs0_size,sizeof(ctx->header.fs_headers[0].pfs0_superblock.pfs0_size));
	sha_update(pfs0_section_sha_ctx,&pfs0_section_zeros,sizeof(pfs0_section_zeros));
	sha_get_hash(pfs0_section_sha_ctx,pfs0_section_hash_result);
	memcpy(ctx->header.section_hashes[0],pfs0_section_hash_result,32);
	free(pfs0_section_hash_result);

}

void meta_save(nca_ctx_t *ctx, pfs0_t *pfs0)
{
	// Decrypt key area to get keys and encrypt new pfs0
	nca_decrypt_key_area(ctx);
	ctx->section_contexts[0].aes = new_aes_ctx(ctx->decrypted_keys[2], 16, AES_MODE_CTR);
	ctx->section_contexts[0].offset = media_to_real(ctx->header.section_entries[0].media_start_offset);
	nca_update_ctr(ctx->section_contexts[0].ctr, ctx->section_contexts[0].offset);
	aes_setiv(ctx->section_contexts[0].aes, ctx->section_contexts[0].ctr, 0x10);
	aes_encrypt(ctx->section_contexts[0].aes, pfs0, pfs0, sizeof(*pfs0));

	fseeko64(ctx->file, 0xC00, SEEK_SET);
	if (!fwrite(pfs0, sizeof(*pfs0) , 1, ctx->file)) {
		fprintf(stderr,"Unable to write meta");
		exit(EXIT_FAILURE);
	}
}
void nca_process(nca_ctx_t *ctx, char *filepath) {
    /* Decrypt header */
    if (!nca_decrypt_header(ctx)) {
        fprintf(stderr, "Invalid NCA header! Are keys correct?\n");
        return;
    }

    /* Sort out crypto type. */
    ctx->crypto_type = ctx->header.crypto_type;
    if (ctx->header.crypto_type2 > ctx->header.crypto_type)
        ctx->crypto_type = ctx->header.crypto_type2;

    if (ctx->crypto_type)
        ctx->crypto_type--; /* 0, 1 are both master key 0. */

    // Set distribution type to "System"
    ctx->header.distribution = 0;
    ctx->format_version = NCAVERSION_NCA3;

    // Set required values for creating .cnmt.xml
    int index = nca_type_to_index(ctx->header.content_type);
    cnmt_xml.contents[index].type = nca_get_content_type(ctx);
    cnmt_xml.contents[index].keygeneration = ctx->crypto_type;
    if (index == 3) {
    	char *tid = (char*)calloc(1,17);
    	//Convert tile id to hex
    	sprintf(tid, "%016" PRIx64, ctx->header.title_id);
    	cnmt_xml.tid = tid;
    	cnmt_xml.filepath = (char*)calloc(1,strlen(filepath) + 1);
    	strcpy(cnmt_xml.filepath,filepath);
    	//Remove .nca and replace it with .xml
    	strip_ext(cnmt_xml.filepath);
    	strcat(cnmt_xml.filepath,".xml");

    	meta_process(ctx);
    }

    /* Re-encrypt header */
    nca_encrypt_header(ctx);
    printf("Patching %s\n",filepath);
    nca_save(ctx);

    // Calculate SHA-256 hash for .cnmt.xml
	sha_ctx_t *sha_ctx = new_sha_ctx(HASH_TYPE_SHA256,0);

	// Get file size
	FILE *file = fopen(filepath, "rb");
	if (file == NULL) {
	    fprintf(stderr, "Failed to open %s!\n", filepath);
	    exit(EXIT_FAILURE);
	}
	fseeko64(file,0,SEEK_END);
	uint64_t filesize = (uint64_t)ftello64(file);
	fseeko64(file,0,SEEK_SET);

    uint64_t read_size = 0x4000000; // 4 MB buffer.
	unsigned char *buf = malloc(read_size);
	if (buf == NULL) {
	    fprintf(stderr, "Failed to allocate file-read buffer!\n");
	    exit(EXIT_FAILURE);
	}

	uint64_t ofs = 0;
	while (ofs < filesize) {
	    if (ofs + read_size >= filesize) read_size = filesize - ofs;
	    if (fread(buf, 1, read_size, file) != read_size) {
	        fprintf(stderr, "Failed to read file!\n");
	        exit(EXIT_FAILURE);
	    }
	    sha_update(sha_ctx,buf,read_size);
	    ofs += read_size;
	}
	unsigned char *hash_result = (unsigned char*)calloc(1,33);
	sha_get_hash(sha_ctx,hash_result);

	// Set file size for creating .cnmt.xml
	cnmt_xml.contents[index].size = filesize;
	// Set file size for creating nsp
	nsp_create_info[index].filesize = filesize;

	// Set filepath for creating nsp
	nsp_create_info[index].filepath = (char*)calloc(1,strlen(filepath) + 1);
	strcpy(nsp_create_info[index].filepath,filepath);


	// Convert hash to hex string
	char *hash_hex = (char*)calloc(1,65);
	hexBinaryString(hash_result,32,hash_hex,65);
	cnmt_xml.contents[index].hash = hash_hex;

	// Get id for creating .cnmt.xml, id = first 16 bytes of hash
	strncpy(cnmt_xml.contents[index].id,hash_hex,32);

	// Set new filename for creating nsp
	if (index == 3) {
		nsp_create_info[index].nsp_filename = (char*)calloc(1,42);
		strcpy(nsp_create_info[index].nsp_filename,cnmt_xml.contents[index].id);
		strcat(nsp_create_info[index].nsp_filename,".cnmt.nca");
	}
	else {
		nsp_create_info[index].nsp_filename = (char*)calloc(1,37);
		strcpy(nsp_create_info[index].nsp_filename,cnmt_xml.contents[index].id);
		strcat(nsp_create_info[index].nsp_filename,".nca");
	}

	// Set required values for creating application.cnmt
	if (index != 3)
	{
		uint8_t cnmt_type = nca_type_to_cnmt_type(index);
		uint8_t padding = 0;
		memcpy(&application_cnmt_contents[index].hash,hash_result,32);
		memcpy(&application_cnmt_contents[index].ncaid,hash_result,16);
		memcpy(&application_cnmt_contents[index].size,&filesize,6);
		memcpy(&application_cnmt_contents[index].type,&cnmt_type,1);
		memcpy(&application_cnmt_contents[index].padding, &padding ,1);
	}

	fclose(file);
	free(buf);
	free_sha_ctx(sha_ctx);
	free(hash_result);
}

void nca_decrypt_key_area(nca_ctx_t *ctx) {
    if (ctx->format_version == NCAVERSION_NCA0_BETA || ctx->format_version == NCAVERSION_NCA0) return;
    aes_ctx_t *aes_ctx = new_aes_ctx(ctx->tool_ctx->settings.keyset.key_area_keys[ctx->crypto_type][ctx->header.kaek_ind], 16, AES_MODE_ECB);
    aes_decrypt(aes_ctx, ctx->decrypted_keys, ctx->header.encrypted_keys, 0x40);
    free_aes_ctx(aes_ctx);
}

/* Decrypt NCA header. */
int nca_decrypt_header(nca_ctx_t *ctx) {
    fseeko64(ctx->file, 0, SEEK_SET);
    if (fread(&ctx->header, 1, 0xC00, ctx->file) != 0xC00) {
        fprintf(stderr, "Failed to read NCA header!\n");
        return 0;
    }
    ctx->is_decrypted = 0;
    
    nca_header_t dec_header;
    
    aes_ctx_t *hdr_aes_ctx = new_aes_ctx(ctx->tool_ctx->settings.keyset.header_key, 32, AES_MODE_XTS);
    aes_xts_decrypt(hdr_aes_ctx, &dec_header, &ctx->header, 0x400, 0, 0x200);
    
    
    if (dec_header.magic == MAGIC_NCA3) {
        ctx->format_version = NCAVERSION_NCA3;
        aes_xts_decrypt(hdr_aes_ctx, &dec_header, &ctx->header, 0xC00, 0, 0x200);
        ctx->header = dec_header;
    } else {
    	fprintf(stderr, "Invalid NCA magic!\n");
    	return 0;
    }
    free_aes_ctx(hdr_aes_ctx);
    return ctx->format_version != NCAVERSION_UNKNOWN;
}

// Encrypt NCA header
void nca_encrypt_header(nca_ctx_t *ctx) {
	nca_header_t enc_header;
	aes_ctx_t *hdr_aes_ctx = new_aes_ctx(ctx->tool_ctx->settings.keyset.header_key, 32, AES_MODE_XTS);
	aes_xts_encrypt(hdr_aes_ctx, &enc_header, &ctx->header, 0xC00, 0, 0x200);
	ctx->header = enc_header;
	free_aes_ctx(hdr_aes_ctx);
}
