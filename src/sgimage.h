#ifndef SGIMAGE_H
#define SGIMAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "sgbitmap.h"

struct SgImageData {
    int width, height;
    int rMask, gMask, bMask, aMask;
    uint32_t *data;
};

void delete_sg_image_data(struct SgImageData *data);

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

class SgImage {
	public:
		SgImage(int id, FILE *file, bool includeAlpha);
		~SgImage();
		int32_t invertOffset() const;
		int bitmapId() const;
                SgBitmap *getParent() const;
		void setInvertImage(SgImage *invert);
		void setParent(SgBitmap *parent);
		struct SgImageData *getImageData(const char *filename);
		char *errorMessage() const;
                bool isExtern() const;

		uint16_t getWidth() const;
		uint16_t getHeight() const;
		uint32_t getOffset() const;
		uint32_t getLength() const;
		uint16_t getType() const;
		
	private:
		uint8_t *fillBuffer(FILE *file);
		/* Image loaders */
		void loadPlainImage(uint32_t *pixels, const uint8_t *buffer);
		void loadIsometricImage(uint32_t *pixels, const uint8_t *buffer);
		void loadSpriteImage(uint32_t *pixels, const uint8_t *buffer);
		void loadAlphaMask(uint32_t *pixels, const uint8_t *buffer);
		
		/* Image decoding methods */
		void writeIsometricBase(uint32_t *pixels, const uint8_t *buffer);
		void writeIsometricTile(uint32_t *pixels, const uint8_t *buffer,
			int offset_x, int offset_y, int tile_width, int tile_height);
		void writeTransparentImage(uint32_t *pixels, const uint8_t *buffer, int length);
		
		/* Pixel setting */
		void set555Pixel(uint32_t *pixels, int x, int y, uint16_t color);
		void setAlphaPixel(uint32_t *pixels, int x, int y, uint8_t color);
		
		/* Error handling */
		void setError(const char *msg);

                void mirrorResult(uint32_t *pixels);
		
		SgImageRecord *record;
		SgImageRecord *workRecord;
		SgBitmap *parent;
		char *error;
		bool invert;
		int imageId;
};

#endif /* SGIMAGE_H */
