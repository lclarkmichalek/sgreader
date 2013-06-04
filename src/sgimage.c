#include "sgimage.h"
#include "utils.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum {
	ISOMETRIC_TILE_WIDTH = 58,
	ISOMETRIC_TILE_HEIGHT = 30,
	ISOMETRIC_TILE_BYTES = 1800,
	ISOMETRIC_LARGE_TILE_WIDTH = 78,
	ISOMETRIC_LARGE_TILE_HEIGHT = 40,
	ISOMETRIC_LARGE_TILE_BYTES = 3200
};

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
    struct SgImageRecord *record;
    struct SgImageRecord *workRecord;
    struct SgBitmap *parent;
    char *error;
    bool invert;
    uint32_t imageId;
};

/* IO */
uint8_t *fillBuffer(struct SgImage *img, FILE *file);

/* Image loaders */
void loadPlainImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer);
void loadIsometricImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer);
void loadSpriteImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer);
void loadAlphaMask(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer);
		
/* Image decoding methods */
void writeIsometricBase(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer);
void writeIsometricTile(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer,
			int offset_x, int offset_y, int tile_width, int tile_height);
void writeTransparentImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer, int length);
		
/* Pixel setting */
void set555Pixel(struct SgImage *img, uint32_t *pixels, int x, int y, uint16_t color);
void setAlphaPixel(struct SgImage *img, uint32_t *pixels, int x, int y, uint8_t color);
		
/* Error handling */
void setError(struct SgImage *img, const char *msg);

/* Misc */
void mirrorResult(struct SgImage *img, uint32_t *pixels);


void sg_delete_image_data(struct SgImageData *data) {
	if (data->data != NULL)
		free(data->data);
	if (data != NULL)
		free(data);
}

struct SgImageRecord sg_read_image_record(FILE *f, bool includeAlpha) {
	struct SgImageRecord rec;
	readUInt32le(f, &rec.offset);
	readUInt32le(f, &rec.length);
	readUInt32le(f, &rec.uncompressed_length);
	fseek(f, 4, SEEK_CUR);
	readInt32le(f, &rec.invert_offset);
	readInt16le(f, &rec.width);
	readInt16le(f, &rec.height);
	fseek(f, 26, SEEK_CUR);
	readUInt16le(f, &rec.type);
	fread(rec.flags, 4, 1, f);
	readUInt8le(f, &rec.bitmap_id);
	fseek(f, 7, SEEK_CUR);
		
	if (includeAlpha) {
		readUInt32le(f, &rec.alpha_offset);
		readUInt32le(f, &rec.alpha_length);
	} else {
		rec.alpha_offset = rec.alpha_length = 0;
	}
	return rec;
}

struct SgImage *sg_read_image(int id, FILE *file, bool includeAlpha) {
	struct SgImage *img = (struct SgImage*)malloc(sizeof(struct SgImage));
	img->parent = NULL;
	img->error = NULL;

	img->imageId = id;
	img->record = img->workRecord = (struct SgImageRecord*)malloc(sizeof(struct SgImageRecord));
	*img->record = sg_read_image_record(file, includeAlpha);
	if (img->record->invert_offset) {
		img->invert = true;
	} else {
		img->invert = false;
	}
	return img;
}

void sg_delete_image(struct SgImage *img) {
	if (img->error != NULL)
		free(img->error);
	if (img->record)
		free(img->record);
	// workRecord is deleted by whoever owns it
	free(img);
}

uint32_t sg_get_image_offset(struct SgImage *img) {
	return img->workRecord->offset;
}

uint32_t sg_get_image_length(struct SgImage *img) {
	return img->workRecord->length;
}

uint32_t sg_get_image_uncompressed_length(struct SgImage *img) {
	return img->workRecord->uncompressed_length;
}

uint32_t sg_get_image_invert_offset(struct SgImage *img) {
	return img->workRecord->invert_offset;
}

uint16_t sg_get_image_width(struct SgImage *img) {
	return img->workRecord->width;
}

uint16_t sg_get_image_height(struct SgImage *img) {
	return img->workRecord->height;
}

uint16_t sg_get_image_type(struct SgImage *img) {
	return img->workRecord->type;
}

bool sg_get_image_extern(struct SgImage *img) {
	return img->workRecord->flags[0];
}

uint8_t sg_get_image_bitmapid(struct SgImage *img) {
	return img->workRecord->bitmap_id;
}

uint32_t sg_get_image_alpha_offset(struct SgImage *img) {
	return img->workRecord->alpha_offset;
}

uint32_t sg_get_image_alpha_length(struct SgImage *img) {
	return img->workRecord->alpha_length;
}

uint32_t sg_get_image_id(struct SgImage *img) {
	return img->imageId;
}

char *sg_get_image_error(struct SgImage *img) {
	return strdup(img->error);
}

struct SgBitmap *sg_get_image_parent(struct SgImage *img) {
	return img->parent;
}

void sg_set_image_invert(struct SgImage *img, struct SgImage *invert) {
	img->workRecord = invert->record;
}

void sg_set_image_parent(struct SgImage *img, struct SgBitmap *parent) {
	img->parent = parent;
}

void setError(struct SgImage *img, const char *message) {
	printf("SGErr: %s\n", message);
	if (img->error != NULL)
		free(img->error);
	img->error = strdup(message);
}

struct SgImageData *sg_load_image_data(struct SgImage *img, const char *filename555) {
	// START DEBUG ((
	/*
	if ((imageId >= 359 && imageId <= 368) || imageId == 459) {
		qDebug("Record %d", imageId);
		qDebug("  offet %d; length %d; length2 %d", record->offset, record->length, record->uncompressed_length);
		qDebug("  invert %d; width %d; height %d", record->invert_offset, record->width, record->height);
		qDebug("  type %d; flags %d %d %d %d; bitmap %d", record->type,
			record->flags[0], record->flags[1], record->flags[2], record->flags[3], record->bitmap_id);
	}
	*/
	// END DEBUG ))
	// Trivial checks
	if (!img->parent) {
		setError(img, "Image has no bitmap parent");
		return NULL;
	}
	if (img->workRecord->width <= 0 || img->workRecord->height <= 0) {
		setError(img, "Width or height invalid");
		return NULL;
	} else if (img->workRecord->length <= 0) {
		setError(img, "No image data available");
		return NULL;
	}
	
        FILE *file555 = fopen(filename555, "rb");
        if (file555 == NULL) {
		setError(img, "Unable to open 555 file");
		return NULL;
        }

	uint8_t *buffer = fillBuffer(img, file555);
	fclose(file555);
	if (buffer == NULL) {
		// Don't set error, as error already set in fillBuffer()
		return NULL;
	}

	uint32_t *pixels = (uint32_t*)(malloc(img->workRecord->width * img->workRecord->height *
					      sizeof(uint32_t)));
	int i;
	for (i = 0; i < img->workRecord->width * img->workRecord->height; i++)
		pixels[i] = 0;

	switch (img->workRecord->type) {
		case 0:
		case 1:
		case 10:
		case 12:
		case 13:
			loadPlainImage(img, pixels, buffer);
			break;
		
		case 30:
			loadIsometricImage(img, pixels, buffer);
			break;
		
		case 256:
		case 257:
		case 276:
			loadSpriteImage(img, pixels, buffer);
			break;
		
		default:
			setError(img, "Unknown image type");
			return NULL;
	}
	
	if (img->workRecord->alpha_length) {
		uint8_t *alpha_buffer = &(buffer[img->workRecord->length]);
		loadAlphaMask(img, pixels, alpha_buffer);
	}
	
	free(buffer);
	
	if (img->invert) {
		mirrorResult(img, pixels);
	}

	struct SgImageData *result = (struct SgImageData*)malloc(sizeof(struct SgImageData));
	result->width = img->workRecord->width;
	result->height = img->workRecord->height;
	result->rMask = 0xff;
	result->bMask = 0xff00;
	result->gMask = 0xff0000;
	result->aMask = 0xff000000;
	result->data = pixels;
	return result;
}

uint8_t* fillBuffer(struct SgImage *img, FILE *file) {
	int data_length = img->workRecord->length + img->workRecord->alpha_length;
	if (data_length <= 0) {
		setError(img, "Data length < 0");
	}
	uint8_t *buffer = (uint8_t*)(malloc(data_length * sizeof(uint8_t)));
	
	// Somehow externals have 1 byte added to their offset
	fseek(file, img->workRecord->offset - img->workRecord->flags[0], SEEK_SET);
	
	int data_read = (int)fread(buffer, 1, data_length, file);
	if (data_length != data_read) {
		if (data_read + 4 == data_length && feof(file)) {
			// Exception for some C3 graphics: last image is 'missing' 4 bytes
			buffer[data_read] = buffer[data_read+1] = 0;
			buffer[data_read+2] = buffer[data_read+3] = 0;
		} else {
			setError(img, "Unable to read from file");
			free(buffer);
			return NULL;
		}
	}
	return buffer;
}

void loadPlainImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer) {
	// Check whether the image data is OK
	if (img->workRecord->height * img->workRecord->width * 2 != (int)img->workRecord->length) {
		setError(img, "Image data length doesn't match image size");
		return;
	}
	
	int i = 0;
        int x, y;
	for (y = 0; y < (int)img->workRecord->height; y++) {
		for (x = 0; x < (int)img->workRecord->width; x++, i+= 2) {
			set555Pixel(img, pixels, x, y, buffer[i] | (buffer[i+1] << 8));
		}
	}
}

void loadIsometricImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer) {
	
	writeIsometricBase(img, pixels, buffer);
	writeTransparentImage(img, pixels, &buffer[img->workRecord->uncompressed_length],
			      img->workRecord->length - img->workRecord->uncompressed_length);
}

void loadSpriteImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer) {
	writeTransparentImage(img, pixels, buffer, img->workRecord->length);
}

void loadAlphaMask(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer) {
	int i = 0;
	int x = 0, y = 0, j;
	int width = img->workRecord->width;
	int length = img->workRecord->alpha_length;
	
	while (i < length) {
		uint8_t c = buffer[i++];
		if (c == 255) {
			/* The next byte is the number of pixels to skip */
			x += buffer[i++];
			while (x >= width) {
				y++; x -= width;
			}
		} else {
			/* `c' is the number of image data bytes */
			for (j = 0; j < c; j++, i++) {
				setAlphaPixel(img, pixels, x, y, buffer[i]);
				x++;
				if (x >= width) {
					y++; x = 0;
				}
			}
		}
	}
}

void writeIsometricBase(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer) {
	int i = 0, x, y;
	int width, height, height_offset;
	int size = img->workRecord->flags[3];
	int x_offset, y_offset;
	int tile_bytes, tile_height, tile_width;
	
	width = img->workRecord->width;
	height = (width + 2) / 2; /* 58 -> 30, 118 -> 60, etc */
	height_offset = img->workRecord->height - height;
	y_offset = height_offset;
	
	if (size == 0) {
		/* Derive the tile size from the height (more regular than width) */
		/* Note that this causes a problem with 4x4 regular vs 3x3 large: */
		/* 4 * 30 = 120; 3 * 40 = 120 -- give precedence to regular */
		if (height % ISOMETRIC_TILE_HEIGHT == 0) {
			size = height / ISOMETRIC_TILE_HEIGHT;
		} else if (height % ISOMETRIC_LARGE_TILE_HEIGHT == 0) {
			size = height / ISOMETRIC_LARGE_TILE_HEIGHT;
		}
	}
	
	/* Determine whether we should use the regular or large (emperor) tiles */
	if (ISOMETRIC_TILE_HEIGHT * size == height) {
		/* Regular tile */
		tile_bytes  = ISOMETRIC_TILE_BYTES;
		tile_height = ISOMETRIC_TILE_HEIGHT;
		tile_width  = ISOMETRIC_TILE_WIDTH;
	} else if (ISOMETRIC_LARGE_TILE_HEIGHT * size == height) {
		/* Large (emperor) tile */
		tile_bytes  = ISOMETRIC_LARGE_TILE_BYTES;
		tile_height = ISOMETRIC_LARGE_TILE_HEIGHT;
		tile_width  = ISOMETRIC_LARGE_TILE_WIDTH;
	} else {
		setError(img, "Unknown tile size");
		return;
	}
	
	/* Check if buffer length is enough: (width + 2) * height / 2 * 2bpp */
	if ((width + 2) * height != (int)img->workRecord->uncompressed_length) {
		setError(img, "Data length doesn't match footprint size");
		return;
	}
	
	i = 0;
	for (y = 0; y < (size + (size - 1)); y++) {
		x_offset = (y < size ? (size - y - 1) : (y - size + 1)) * tile_height;
		for (x = 0; x < (y < size ? y + 1 : 2 * size - y - 1); x++, i++) {
			writeIsometricTile(img, pixels, &buffer[i * tile_bytes],
					   x_offset, y_offset, tile_width, tile_height);
			x_offset += tile_width + 2;
		}
		y_offset += tile_height / 2;
	}
	
}

void writeIsometricTile(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer,
			int offset_x, int offset_y, int tile_width, int tile_height) {
	int half_height = tile_height / 2;
	int x, y, i = 0;
	
	for (y = 0; y < half_height; y++) {
		int start = tile_height - 2 * (y + 1);
		int end = tile_width - start;
		for (x = start; x < end; x++, i += 2) {
			set555Pixel(img, pixels, offset_x + x, offset_y + y,
				    (buffer[i+1] << 8) | buffer[i]);
		}
	}
	for (y = half_height; y < tile_height; y++) {
		int start = 2 * y - tile_height;
		int end = tile_width - start;
		for (x = start; x < end; x++, i += 2) {
			set555Pixel(img, pixels, offset_x + x, offset_y + y,
				(buffer[i+1] << 8) | buffer[i]);
		}
	}
}

void writeTransparentImage(struct SgImage *img, uint32_t *pixels, const uint8_t *buffer, int length) {
	int i = 0;
	int x = 0, y = 0, j;
	int width = img->workRecord->width;
	
	while (i < length) {
		uint8_t c = buffer[i++];
		if (c == 255) {
			/* The next byte is the number of pixels to skip */
			x += buffer[i++];
			while (x >= width) {
				y++; x -= width;
			}
		} else {
			/* `c' is the number of image data bytes */
			for (j = 0; j < c; j++, i += 2) {
				set555Pixel(img, pixels, x, y, buffer[i] | (buffer[i+1] << 8));
				x++;
				if (x >= width) {
					y++; x = 0;
				}
			}
		}
	}
}

void set555Pixel(struct SgImage *img, uint32_t *pixels, int x, int y, uint16_t color) {
	if (color == 0xf81f) {
		return;
	}
	
	uint32_t rgb = 0xff000000;
	
	// Red: bits 11-15, should go to bits 17-24
	rgb |= ((color & 0x7c00) << 9) | ((color & 0x7000) << 4);
	
	// Green: bits 6-10, should go to bits 9-16
	rgb |= ((color & 0x3e0) << 6) | ((color & 0x300));
	
	// Blue: bits 1-5, should go to bits 1-8
	rgb |= ((color & 0x1f) << 3) | ((color & 0x1c) >> 2);
	
	pixels[y * img->workRecord->width + x] = rgb;
}

void setAlphaPixel(struct SgImage *img, uint32_t *pixels, int x, int y, uint8_t color) {
	/* Only the first five bits of the alpha channel are used */
	uint8_t alpha = ((color & 0x1f) << 3) | ((color & 0x1c) >> 2);

	int p = y * img->workRecord->width + x;
	pixels[p] = (pixels[p] & 0x00ffffff) | (alpha << 24);
}

void mirrorResult(struct SgImage *img, uint32_t *pixels) {
	int x, y;
	for (x = 0; x < (img->workRecord->width - 1) / 2; x++) {
		for (y = 0; y < img->workRecord->height; y++) {
			int p1 = y * img->workRecord->width + x;
			int p2 = (y + 1) * img->workRecord->width - x;
			uint32_t tmp;
			tmp = pixels[p1];
			pixels[p1] = pixels[p2];
			pixels[p2] = tmp;
		}
	}
}
