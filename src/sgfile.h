#ifndef SGFILE_H
#define SGFILE_H

#include <stdio.h>

#include "./sgimage.h"
#include "./sgbitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SgFile;

struct SgFile *sg_read_file(const char *filename);
void sg_delete_file(struct SgFile *file);

uint32_t sg_get_file_version(struct SgFile *file);
uint32_t sg_get_file_total_filesize(struct SgFile *file);
uint32_t sg_get_file_555_filesize(struct SgFile *file);
uint32_t sg_get_file_external_filesize(struct SgFile *file);

int sg_get_file_bitmap_count(struct SgFile *file);
void sg_add_file_bitmap(struct SgFile *file, struct SgBitmap *bmp);
struct SgBitmap *sg_get_file_bitmap(struct SgFile *file, int i);

int sg_get_file_image_count(struct SgFile *file);
void sg_add_file_image(struct SgFile *file, struct SgImage *bmp);
struct SgImage *sg_get_file_image(struct SgFile *file, int i);

#ifdef __cplusplus
}
#endif

#endif /* SGFILE_H */
