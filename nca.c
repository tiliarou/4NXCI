#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <inttypes.h>
#include "nsp.h"
#include "nca.h"
#include "aes.h"
#include "pki.h"
#include "sha.h"
#include "rsa.h"
#include "utils.h"
#include "extkeys.h"
#include "filepath.h"

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
int nca_type_to_inex(uint8_t nca_type)
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
    int index = nca_type_to_inex(ctx->header.content_type);
    cnmt_xml.contents[index].type = nca_get_content_type(ctx);
    cnmt_xml.contents[index].keygeneration = (char)ctx->crypto_type;
    if (index == 3) {
    	char *tid = (char*)malloc(17);
    	//Convert tile id to hex
    	sprintf(tid, "%016" PRIx64, (uint64_t)ctx->header.title_id);
    	cnmt_xml.tid = tid;
    	cnmt_xml.filepath = (char*)malloc(strlen(filepath) + 1);
    	strcpy(cnmt_xml.filepath,filepath);
    	//Remove .nca and replace it with .xml
    	strip_ext(cnmt_xml.filepath);
    	strcat(cnmt_xml.filepath,".xml");
    }

    /* Re-encrypt header */
    nca_encrypt_header(ctx);
    printf("Patchig %s\n",filepath);
    nca_save(ctx);

    // Calculate SHA-256 hash for .cnmt.xml
	sha_ctx_t *sha_ctx = new_sha_ctx(HASH_TYPE_SHA256,0);

	// Get file size
	FILE *file = fopen(filepath, "rb");
	fseeko64(file,0,SEEK_END);
	uint64_t filesize = (uint64_t)ftello64(file);
	fseeko64(file,0,SEEK_SET);

	// Set file size for creating .cnmt.xml
	cnmt_xml.contents[index].size = filesize;
	// Set file size for creating nsp
	nsp_create_info[index].filesize = filesize;

	// Set filepath for creating nsp
	nsp_create_info[index].filepath = (char*)malloc(strlen(filepath) + 1);
	strcpy(nsp_create_info[index].filepath,filepath);

	if (file == NULL) {
	    fprintf(stderr, "Failed to open %s!\n", filepath);
	    exit(EXIT_FAILURE);
	}

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
	unsigned char *hash_result = (unsigned char*)malloc(33);
	sha_get_hash(sha_ctx,hash_result);

	// Convert hash to hex string
	char* hash_hex = (char*)malloc(65);
	hexBinaryString(hash_result,32,hash_hex,65);
	cnmt_xml.contents[index].hash = hash_hex;

	// Get id for creating .cnmt.xml
	char *id = (char*)malloc(strlen(basename(filepath)) + 1);
	strcpy(id,basename(filepath));
	strip_ext(id);
	// Remove .cnmt
	if (index == 3)
		strip_ext(id);

	cnmt_xml.contents[index].id = id;

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
