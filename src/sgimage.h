#ifndef SGIMAGE_H
#define SGIMAGE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "./sgbitmap.h"

struct SgImageData {
    int width, height;
    int rMask, gMask, bMask, aMask;
    uint32_t *data;
};

void delete_sg_image_data(SgImageData *data);

struct SgImageRecord {
	uint32_t offset;
	uint32_t length;
	uint32_t uncompressed_length;
	/* 4 zero bytes: */
	int32_t invert_offset;
	int16_t width;
	int16_t height;
	/* 26 unknown bytes, mostly zero, first four are 2 shorts */
	uint16_t type;
	/* 4 flag/option-like bytes: */
	char flags[4];
	uint8_t bitmap_id;
	/* 3 bytes + 4 zero bytes */
	/* For D6 and up SG3 versions: alpha masks */
	uint32_t alpha_offset;
	uint32_t alpha_length;
};

struct SgImage {
	SgImageRecord *record;
	SgImageRecord *workRecord;
	SgBitmap *parent;
	char *error;
	bool invert;
	int imageId;
};

struct SgImage *read_sg_image(int id, FILE *file, bool includeAlpha);
void delete_sg_image(struct SgImage *img);
void set_sg_image_invert(struct SgImage *img, struct SgImage *invert);
struct SgImageData *get_sg_image_data(struct SgImage *img, const char *filename);
bool is_sg_image_extern(struct SgImage *img);

#endif /* SGIMAGE_H */
