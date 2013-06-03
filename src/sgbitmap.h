#ifndef SGBITMAP_H
#define SGBITMAP_H

#include <stdbool.h>
#include <stdio.h>

#include "./sgimage.h"

struct SgBitmapRecord {
	char filename[65];
	char comment[51];
	uint32_t width;
	uint32_t height;
	uint32_t num_images;
	uint32_t start_index;
	uint32_t end_index;
	/* 4 bytes - quint32 between start & end */
	/* 16b, 4x int with unknown purpose */
	/*  8b, 2x int with (real?) width & height */
	/* 12b, 3x int: if any is non-zero: internal image */
	/* 24 more misc bytes, most zero */
};
struct SgImage;

struct SgBitmap {
	struct SgImage **images;
	int images_n;
	int images_c;
	struct SgBitmapRecord *record;
	char *sgFilename;
	int bitmapId;
};

struct SgBitmap *read_sg_bitmap(int id, const char *sgFilename, FILE *file);
void delete_sg_bitmap(struct SgBitmap *bmp);
void add_sg_bitmap_image(struct SgBitmap *bmp, struct SgImage *child);
bool is_sg_bitmap_extern(struct SgBitmap *bmp);

#endif /* SGBITMAP_H */
