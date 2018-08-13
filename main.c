#include <getopt.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nsp.h"
#include "types.h"
#include "utils.h"
#include "settings.h"
#include "pki.h"
#include "xci.h"
#include "extkeys.h"
#include "cnmt.h"
#include "version.h"

/* 4NXCI by The-4n
   Based on hactool by SciresM
   */
cnmt_xml_t cnmt_xml;
nsp_create_info_t nsp_create_info[7];
application_cnmt_content_t application_cnmt_contents[3];

// Print Usage
static void usage(void) {
    fprintf(stderr, 
    	"4NXCI %s by The-4n\n"
        "Usage: %s <filename.xci>\n"
    	"Make sure to put your keyset in keys.dat\n", NXCI_VERSION, USAGE_PROGRAM_NAME);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	//Compiler nag
	(void)argc;

    nxci_ctx_t tool_ctx;
    char input_name[0x200];
    filepath_t keypath;;

    memset(&tool_ctx, 0, sizeof(tool_ctx));
    memset(input_name, 0, sizeof(input_name));
    memset(&cnmt_xml,0,sizeof(cnmt_xml));
    memset(&cnmt_xml,0,sizeof(application_cnmt_contents));
    filepath_init(&keypath);

    pki_initialize_keyset(&tool_ctx.settings.keyset, KEYSET_RETAIL);

    // Hardcode keyfile path
    filepath_set(&keypath, "keys.dat");
    
    // Try to populate default keyfile.
    FILE *keyfile = NULL;
    keyfile = os_fopen(keypath.os_path, OS_MODE_READ);

    if (keyfile != NULL) {
        extkeys_initialize_keyset(&tool_ctx.settings.keyset, keyfile);
        pki_derive_keys(&tool_ctx.settings.keyset);
        fclose(keyfile);
    }
    else {
    	fprintf(stderr,"unable to open keys.dat\n"
    			"make sure to put your keyset in keys.dat\n");
    	return EXIT_FAILURE;
    }
    
    if (argv[1] != NULL) {
        // Copy input file.
        strncpy(input_name, argv[1], sizeof(input_name));
    } else
        usage();
    
    if (!(tool_ctx.file = fopen(input_name, "rb"))) {
        fprintf(stderr, "unable to open %s: %s\n", input_name, strerror(errno));
        return EXIT_FAILURE;
    }
    
    xci_ctx_t xci_ctx;
    memset(&xci_ctx, 0, sizeof(xci_ctx));
    xci_ctx.file = tool_ctx.file;
    xci_ctx.tool_ctx = &tool_ctx;

    // Hardcode secure partition save path to "4nxci_extracted_nsp" directory
    filepath_set(&xci_ctx.tool_ctx->settings.secure_dir_path, "4nxci_extracted_xci");

    xci_process(&xci_ctx);

    create_cnmt_xml();
    create_dummy_cert(xci_ctx.tool_ctx->settings.secure_dir_path);
    create_dummy_tik(xci_ctx.tool_ctx->settings.secure_dir_path);
    create_nsp();

    fclose(tool_ctx.file);
    printf("Done!\n");
    return EXIT_SUCCESS;
}
