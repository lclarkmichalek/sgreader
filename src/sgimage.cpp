#include "sgimage.h"
#include "utils.h"

#include <QFile>
#include <QDataStream>

enum {
	ISOMETRIC_TILE_WIDTH = 58,
	ISOMETRIC_TILE_HEIGHT = 30,
	ISOMETRIC_TILE_BYTES = 1800,
	ISOMETRIC_LARGE_TILE_WIDTH = 78,
	ISOMETRIC_LARGE_TILE_HEIGHT = 40,
	ISOMETRIC_LARGE_TILE_BYTES = 3200
};

class SgImageRecord {
public:
	SgImageRecord(FILE *f, bool includeAlpha) {
		readUInt32le(f, &offset);
		readUInt32le(f, &length);
		readUInt32le(f, &uncompressed_length);
		fseek(f, 4, SEEK_CUR);
		readInt32le(f, &invert_offset);
		readInt16le(f, &width);
		readInt16le(f, &height);
		fseek(f, 26, SEEK_CUR);
		readUInt16le(f, &type);
		fread(flags, 4, 1, f);
		readUInt8le(f, &bitmap_id);
		fseek(f, 7, SEEK_CUR);
		
		if (includeAlpha) {
			readUInt32le(f, &alpha_offset);
			readUInt32le(f, &alpha_length);
		} else {
			alpha_offset = alpha_length = 0;
		}
	}
	
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

void delete_sg_image_data(struct SgImageData *data) {
	if (data->data != NULL)
		free(data->data);
	if (data != NULL)
		free(data);
}

SgImage::SgImage(int id, FILE *file, bool includeAlpha)
	: parent(NULL)
{
	imageId = id;
	record = workRecord = new SgImageRecord(file, includeAlpha);
	if (record->invert_offset) {
		invert = true;
	} else {
		invert = false;
	}
}

SgImage::~SgImage() {
	if (record) delete record;
	// workRecord is deleted by whoever owns it
}

int32_t SgImage::invertOffset() const {
	return record->invert_offset;
}

int SgImage::bitmapId() const {
	if (workRecord) return workRecord->bitmap_id;
	return record->bitmap_id;
}

QString SgImage::description() const {
	return QString("%0x%1")
		.arg(workRecord->width)
		.arg(workRecord->height);
}

QString SgImage::fullDescription() const {
	return QString("ID %7: offset %0, length %1, width %2, height %3, type %5, %6")
		.arg(workRecord->offset)
		.arg(workRecord->length)
		.arg(workRecord->width)
		.arg(workRecord->height)
		.arg(workRecord->type)
		.arg(workRecord->flags[0] ? "external" : "internal")
		.arg(imageId);
}

void SgImage::setInvertImage(SgImage *invert) {
	this->workRecord = invert->record;
}

void SgImage::setParent(SgBitmap *parent) {
	this->parent = parent;
}

SgBitmap* SgImage::getParent() const {
	return this->parent;
}

QString SgImage::errorMessage() const {
	return error;
}

void SgImage::setError(const QString &message) {
	qDebug(message.toAscii().constData());
	error = message;
}

bool SgImage::isExtern() const {
	return bool(workRecord->flags[0]);
}

struct SgImageData *SgImage::getImageData(const char *filename555) {
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
	if (!parent) {
		setError("Image has no bitmap parent");
		return NULL;
	}
	if (workRecord->width <= 0 || workRecord->height <= 0) {
		setError(QString("Width or height invalid (%0 x %1)")
			.arg(workRecord->width).arg(workRecord->height));
		return NULL;
	} else if (workRecord->length <= 0) {
		setError("No image data available");
		return NULL;
	}
	
        FILE *file555 = fopen(filename555, "rb");
        if (file555 == NULL) {
		qDebug("Unable to open 555 file");
		return NULL;
        }

	uint8_t *buffer = fillBuffer(file555);
	fclose(file555);
	if (buffer == NULL) {
		qDebug("Unable to load buffer"); // error already set in fillBuffer()
		return NULL;
	}

	uint32_t *pixels = (uint32_t*)(malloc(workRecord->width * workRecord->height * sizeof(uint32_t)));
	int i;
	for (i = 0; i < workRecord->width * workRecord->height; i++)
		pixels[i] = 0;

	switch (workRecord->type) {
		case 0:
		case 1:
		case 10:
		case 12:
		case 13:
			loadPlainImage(pixels, buffer);
			break;
		
		case 30:
			loadIsometricImage(pixels, buffer);
			break;
		
		case 256:
		case 257:
		case 276:
			loadSpriteImage(pixels, buffer);
			break;
		
		default:
			qWarning("Unknown image type: %d", workRecord->type);
			break;
	}
	
	if (workRecord->alpha_length) {
		uint8_t *alpha_buffer = &(buffer[workRecord->length]);
		loadAlphaMask(pixels, alpha_buffer);
	}
	
	free(buffer);
	
	if (invert) {
		mirrorResult(pixels);
	}

	struct SgImageData *result = (struct SgImageData*)malloc(sizeof(struct SgImageData));
	result->width = workRecord->width;
	result->height = workRecord->height;
	result->rMask = 0xff;
	result->bMask = 0xff00;
	result->gMask = 0xff0000;
	result->aMask = 0xff000000;
	result->data = pixels;
	return result;
}

uint8_t* SgImage::fillBuffer(FILE *file) {
	int data_length = workRecord->length + workRecord->alpha_length;
	if (data_length <= 0) {
		qDebug("Data length: %d", data_length); // not an error per se
	}
	uint8_t *buffer = (uint8_t*)(malloc(data_length * sizeof(uint8_t)));
	if (buffer == NULL) {
		setError(QString("Cannot allocate %0 bytes of memory").arg(data_length));
		return NULL;
	}
	
	// Somehow externals have 1 byte added to their offset
	fseek(file, workRecord->offset - workRecord->flags[0], SEEK_SET);
	
	int data_read = (int)fread(buffer, 1, data_length, file);
	if (data_length != data_read) {
		if (data_read + 4 == data_length && feof(file)) {
			// Exception for some C3 graphics: last image is 'missing' 4 bytes
			buffer[data_read] = buffer[data_read+1] = 0;
			buffer[data_read+2] = buffer[data_read+3] = 0;
		} else {
			setError(QString("Unable to read %0 bytes from file (read %1 bytes)")
				.arg(data_length).arg(data_read));
			free(buffer);
			return NULL;
		}
	}
	return buffer;
}

void SgImage::loadPlainImage(uint32_t *pixels, const uint8_t *buffer) {
	// Check whether the image data is OK
	if (workRecord->height * workRecord->width * 2 != (int)workRecord->length) {
		setError("Image data length doesn't match image size");
		return;
	}
	
	int i = 0;
	for (int y = 0; y < (int)workRecord->height; y++) {
		for (int x = 0; x < (int)workRecord->width; x++, i+= 2) {
			set555Pixel(pixels, x, y, buffer[i] | (buffer[i+1] << 8));
		}
	}
}

void SgImage::loadIsometricImage(uint32_t *pixels, const uint8_t *buffer) {
	
	writeIsometricBase(pixels, buffer);
	writeTransparentImage(pixels, &buffer[workRecord->uncompressed_length],
		workRecord->length - workRecord->uncompressed_length);
}

void SgImage::loadSpriteImage(uint32_t *pixels, const uint8_t *buffer) {
	writeTransparentImage(pixels, buffer, workRecord->length);
}

void SgImage::loadAlphaMask(uint32_t *pixels, const uint8_t *buffer) {
	int i = 0;
	int x = 0, y = 0, j;
	int width = workRecord->width;
	int length = workRecord->alpha_length;
	
	while (i < length) {
		quint8 c = buffer[i++];
		if (c == 255) {
			/* The next byte is the number of pixels to skip */
			x += buffer[i++];
			while (x >= width) {
				y++; x -= width;
			}
		} else {
			/* `c' is the number of image data bytes */
			for (j = 0; j < c; j++, i++) {
				setAlphaPixel(pixels, x, y, buffer[i]);
				x++;
				if (x >= width) {
					y++; x = 0;
				}
			}
		}
	}
}

void SgImage::writeIsometricBase(uint32_t *pixels, const uint8_t *buffer) {
	int i = 0, x, y;
	int width, height, height_offset;
	int size = workRecord->flags[3];
	int x_offset, y_offset;
	int tile_bytes, tile_height, tile_width;
	
	width = workRecord->width;
	height = (width + 2) / 2; /* 58 -> 30, 118 -> 60, etc */
	height_offset = workRecord->height - height;
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
		setError(QString("Unknown tile size: %0 (height %1, width %2, size %3)")
			.arg(2 * height / size).arg(height).arg(width).arg(size));
		return;
	}
	
	/* Check if buffer length is enough: (width + 2) * height / 2 * 2bpp */
	if ((width + 2) * height != (int)workRecord->uncompressed_length) {
		setError(QString(
			"Data length doesn't match footprint size: %0 vs %1 (%2) %3")
			.arg((width + 2) * height)
			.arg(workRecord->uncompressed_length)
			.arg(workRecord->length)
			.arg(workRecord->invert_offset));
		return;
	}
	
	i = 0;
	for (y = 0; y < (size + (size - 1)); y++) {
		x_offset = (y < size ? (size - y - 1) : (y - size + 1)) * tile_height;
		for (x = 0; x < (y < size ? y + 1 : 2 * size - y - 1); x++, i++) {
			writeIsometricTile(pixels, &buffer[i * tile_bytes],
				x_offset, y_offset, tile_width, tile_height);
			x_offset += tile_width + 2;
		}
		y_offset += tile_height / 2;
	}
	
}

void SgImage::writeIsometricTile(uint32_t *pixels, const uint8_t *buffer,
		int offset_x, int offset_y, int tile_width, int tile_height) {
	int half_height = tile_height / 2;
	int x, y, i = 0;
	
	for (y = 0; y < half_height; y++) {
		int start = tile_height - 2 * (y + 1);
		int end = tile_width - start;
		for (x = start; x < end; x++, i += 2) {
			set555Pixel(pixels, offset_x + x, offset_y + y,
				(buffer[i+1] << 8) | buffer[i]);
		}
	}
	for (y = half_height; y < tile_height; y++) {
		int start = 2 * y - tile_height;
		int end = tile_width - start;
		for (x = start; x < end; x++, i += 2) {
			set555Pixel(pixels, offset_x + x, offset_y + y,
				(buffer[i+1] << 8) | buffer[i]);
		}
	}
}

void SgImage::writeTransparentImage(uint32_t *pixels, const uint8_t *buffer, int length) {
	int i = 0;
	int x = 0, y = 0, j;
	int width = workRecord->width;
	
	while (i < length) {
		quint8 c = buffer[i++];
		if (c == 255) {
			/* The next byte is the number of pixels to skip */
			x += buffer[i++];
			while (x >= width) {
				y++; x -= width;
			}
		} else {
			/* `c' is the number of image data bytes */
			for (j = 0; j < c; j++, i += 2) {
				set555Pixel(pixels, x, y, buffer[i] | (buffer[i+1] << 8));
				x++;
				if (x >= width) {
					y++; x = 0;
				}
			}
		}
	}
}

void SgImage::set555Pixel(uint32_t *pixels, int x, int y, uint16_t color) {
	if (color == 0xf81f) {
		return;
	}
	
	quint32 rgb = 0xff000000;
	
	// Red: bits 11-15, should go to bits 17-24
	rgb |= ((color & 0x7c00) << 9) | ((color & 0x7000) << 4);
	
	// Green: bits 6-10, should go to bits 9-16
	rgb |= ((color & 0x3e0) << 6) | ((color & 0x300));
	
	// Blue: bits 1-5, should go to bits 1-8
	rgb |= ((color & 0x1f) << 3) | ((color & 0x1c) >> 2);
	
	pixels[y * workRecord->width + x] = rgb;
}

void SgImage::setAlphaPixel(uint32_t *pixels, int x, int y, uint8_t color) {
	/* Only the first five bits of the alpha channel are used */
	uint8_t alpha = ((color & 0x1f) << 3) | ((color & 0x1c) >> 2);

	int p = y * workRecord->width + x;
	pixels[p] = (pixels[p] & 0x00ffffff) | (alpha << 24);
}

void SgImage::mirrorResult(uint32_t *pixels) {
	int x, y;
	for (x = 0; x < (workRecord->width - 1) / 2; x++) {
		for (y = 0; y < workRecord->height; y++) {
			int p1 = y * workRecord->width + x;
			int p2 = (y + 1) * workRecord->width - x;
			uint32_t tmp;
			tmp = pixels[p1];
			pixels[p1] = pixels[p2];
			pixels[p2] = tmp;
		}
	}
}
