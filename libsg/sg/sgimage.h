#ifndef SGIMAGE_H
#define SGIMAGE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SgFile;
struct SgBitmap;
struct SgImage;

struct SgImage *sg_read_image(int id, FILE *file, bool includeAlpha);
void sg_delete_image(struct SgImage *img);

// Fuck yes, GETTERS!
uint32_t sg_get_image_offset(struct SgImage *img);
uint32_t sg_get_image_length(struct SgImage *img);
uint32_t sg_get_image_uncompressed_length(struct SgImage *img);
uint32_t sg_get_image_invert_offset(struct SgImage *img);
uint16_t sg_get_image_width(struct SgImage *img);
uint16_t sg_get_image_height(struct SgImage *img);
uint16_t sg_get_image_type(struct SgImage *img);
bool sg_get_image_extern(struct SgImage *img);
uint8_t sg_get_image_bitmapid(struct SgImage *img);
uint32_t sg_get_image_alpha_offset(struct SgImage *img);
uint32_t sg_get_image_alpha_length(struct SgImage *img);
uint32_t sg_get_image_id(struct SgImage *img);
// Fresh memory from the sweet fountain of strdup
char *sg_get_image_error(struct SgImage *img);
struct SgBitmap *sg_get_image_parent(struct SgImage *img);

// Fuck no, SETTERS!
void sg_set_image_invert(struct SgImage *img, struct SgImage *invert);
void sg_set_image_parent(struct SgImage *img, struct SgBitmap *parent);

/* The actual image data. The mask's should be checked to confirm that the
   data is in ARGB32 format. But it always will be, most likely, due to me
   having no clue how Pecunia's bitshifting wizardry works.

   If you're using a QImage, you can initialize the QImage via

       struct SgImageData *imgdata;
       QImage((uchar*)imgdata->data, imgdata->width, imgdata->height,
              QImage::Format_ARGB32);

   Though you will need to defer the freeing of the imgdata till the QImage
   is destroyed.
*/
struct SgImageData {
	int width, height;
	int rMask, gMask, bMask, aMask;
	uint32_t *data;
};

void sg_delete_image_data(struct SgImageData *data);
struct SgImageData *sg_load_image_data(struct SgImage *img, const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* SGIMAGE_H */
