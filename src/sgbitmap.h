#ifndef SGBITMAP_H
#define SGBITMAP_H

#include <stdbool.h>
#include <stdio.h>

#include "./sgimage.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SgBitmap;

struct SgBitmap *sg_read_bitmap(int id, const char *sgFilename, FILE *file);
void sg_delete_bitmap(struct SgBitmap *bmp);

const char *sg_get_bitmap_filename(struct SgBitmap *bmp);
const char *sg_get_bitmap_comment(struct SgBitmap *bmp);
uint32_t sg_get_bitmap_width(struct SgBitmap *bmp);
uint32_t sg_get_bitmap_height(struct SgBitmap *bmp);
// Returns the number of images specified in the record, not the number in the
// images array
uint32_t sg_get_bitmap_num_images(struct SgBitmap *bmp);
uint32_t sg_get_bitmap_start_index(struct SgBitmap *bmp);
uint32_t sg_get_bitmap_end_index(struct SgBitmap *bmp);
const char *sg_get_bitmap_sg_filename(struct SgBitmap *bmp);
int sg_get_bitmap_id(struct SgBitmap *bmp);

struct SgImage *sg_get_bitmap_image(struct SgBitmap *bmp, int i);
int sg_get_bitmap_image_count(struct SgBitmap *bmp);
void sg_add_bitmap_image(struct SgBitmap *bmp, struct SgImage *child);

#ifdef __cplusplus
}
#endif

#endif /* SGBITMAP_H */
