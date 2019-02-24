#include <stdio.h>
#include <string.h>
#include "romfs.h"
#include "nacp.h"

static void romfs_visit_file(romfs_ctx_t *ctx, uint32_t file_offset, filepath_t *dir_path, nsp_ctx_t *nsp_ctx)
{
    romfs_fentry_t *entry = romfs_get_fentry(ctx->files, file_offset);
    filepath_t *cur_path = calloc(1, sizeof(filepath_t));
    if (cur_path == NULL)
    {
        fprintf(stderr, "Failed to allocate filepath!\n");
        exit(EXIT_FAILURE);
    }

    filepath_copy(cur_path, dir_path);
    if (entry->name_size)
    {
        filepath_append_n(cur_path, entry->name_size, "%s", entry->name);
    }

    if ((strcmp(cur_path->char_path, "/control.nacp") == 0) || (strcmp(cur_path->char_path, "\\control.nacp") == 0))
    {
        // Read control.nacp
        uint64_t current_pos = ftello64(ctx->file);
        nacp_t nacp;
        memset(&nacp, 0, sizeof(nacp_t));
        fseeko64(ctx->file, ctx->romfs_offset + ctx->header.data_offset + entry->offset, SEEK_SET);
        if (fread(&nacp, 1, sizeof(nacp_t), ctx->file) != sizeof(nacp_t))
        {
            fprintf(stderr, "Failed to read control.nacp from RomFS!\n");
            exit(EXIT_FAILURE);
        }

        // Fill nsp_ctx titlename and version
        char *forbidden_chars[10] = {"/", "\\", "?", "%%", "*", ":", "|", "\"", ">", "<"};
        for (int i = 0; i < 16; i++)
        {
            if (nacp.Title[i].Name[0] != 0x00)
            {
                for (unsigned int j = 0; j < strlen(nacp.Title[i].Name); j++)
                {
                    int found_forbidden_char = 0;
                    for (int k = 0; k < 10; k++)
                    {
                        if (strncmp(&nacp.Title[i].Name[j], forbidden_chars[k], 1) == 0)
                            found_forbidden_char = 1;
                    }
                    if (found_forbidden_char == 0)
                        strncat(nsp_ctx->title_name, &nacp.Title[i].Name[j], 1);
                }
                break;
            }
        }
        strcpy(nsp_ctx->title_display_version, nacp.DisplayVersion);
        fseeko64(ctx->file, current_pos, SEEK_SET);
        return;
    }

    free(cur_path);

    if (entry->sibling != ROMFS_ENTRY_EMPTY)
    {
        romfs_visit_file(ctx, entry->sibling, dir_path, nsp_ctx);
    }
}

static void romfs_visit_dir(romfs_ctx_t *ctx, uint32_t dir_offset, filepath_t *parent_path, nsp_ctx_t *nsp_ctx)
{
    romfs_direntry_t *entry = romfs_get_direntry(ctx->directories, dir_offset);
    filepath_t *cur_path = calloc(1, sizeof(filepath_t));
    if (cur_path == NULL)
    {
        fprintf(stderr, "Failed to allocate filepath!\n");
        exit(EXIT_FAILURE);
    }

    filepath_copy(cur_path, parent_path);
    if (entry->name_size)
    {
        filepath_append_n(cur_path, entry->name_size, "%s", entry->name);
    }

    if (entry->file != ROMFS_ENTRY_EMPTY)
    {
        romfs_visit_file(ctx, entry->file, cur_path, nsp_ctx);
    }
    if (entry->child != ROMFS_ENTRY_EMPTY)
    {
        romfs_visit_dir(ctx, entry->child, cur_path, nsp_ctx);
    }
    if (entry->sibling != ROMFS_ENTRY_EMPTY)
    {
        romfs_visit_dir(ctx, entry->sibling, parent_path, nsp_ctx);
    }

    free(cur_path);
}

void romfs_process(romfs_ctx_t *ctx, nsp_ctx_t *nsp_ctx)
{
    ctx->romfs_offset = 0;
    fseeko64(ctx->file, ctx->romfs_offset, SEEK_SET);
    if (fread(&ctx->header, 1, sizeof(romfs_hdr_t), ctx->file) != sizeof(romfs_hdr_t))
    {
        fprintf(stderr, "Failed to read RomFS header!\n");
        return;
    }

    if (ctx->header.header_size == ROMFS_HEADER_SIZE)
    {
        /* Pre-load the file/data entry caches. */
        ctx->directories = calloc(1, ctx->header.dir_meta_table_size);
        if (ctx->directories == NULL)
        {
            fprintf(stderr, "Failed to allocate RomFS directory cache!\n");
            exit(EXIT_FAILURE);
        }

        fseeko64(ctx->file, ctx->romfs_offset + ctx->header.dir_meta_table_offset, SEEK_SET);
        if (fread(ctx->directories, 1, ctx->header.dir_meta_table_size, ctx->file) != ctx->header.dir_meta_table_size)
        {
            fprintf(stderr, "Failed to read RomFS directory cache!\n");
            exit(EXIT_FAILURE);
        }

        ctx->files = calloc(1, ctx->header.file_meta_table_size);
        if (ctx->files == NULL)
        {
            fprintf(stderr, "Failed to allocate RomFS file cache!\n");
            exit(EXIT_FAILURE);
        }
        fseeko64(ctx->file, ctx->romfs_offset + ctx->header.file_meta_table_offset, SEEK_SET);
        if (fread(ctx->files, 1, ctx->header.file_meta_table_size, ctx->file) != ctx->header.file_meta_table_size)
        {
            fprintf(stderr, "Failed to read RomFS file cache!\n");
            exit(EXIT_FAILURE);
        }
    }

    filepath_t fakepath;
    filepath_init(&fakepath);
    filepath_set(&fakepath, "");
    romfs_visit_dir(ctx, 0, &fakepath, nsp_ctx);
}