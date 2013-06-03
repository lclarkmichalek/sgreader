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

class SgImageRecord;

class SgImage {
	public:
		SgImage(int id, FILE *file, bool includeAlpha);
		~SgImage();
		int32_t invertOffset() const;
		int bitmapId() const;
		QString description() const;
		QString fullDescription() const;
                SgBitmap *getParent() const;
		void setInvertImage(SgImage *invert);
		void setParent(SgBitmap *parent);
		struct SgImageData *getImageData(const char *filename);
		QString errorMessage() const;
                bool isExtern() const;
		
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
		void setError(const QString &message);

                void mirrorResult(uint32_t *pixels);
		
		SgImageRecord *record;
		SgImageRecord *workRecord;
		SgBitmap *parent;
		QString error;
		bool invert;
		int imageId;
};

#endif /* SGIMAGE_H */
