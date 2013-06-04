#include "sgfile.h"
#include "sgimage.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

enum {
	SG_HEADER_SIZE = 680,
	SG_BITMAP_RECORD_SIZE = 200
};

struct SgHeader *sg_read_header(FILE *f) {
	struct SgHeader *h = (struct SgHeader*)malloc(sizeof(struct SgHeader));
	readUInt32le(f, &h->sg_filesize);
	readUInt32le(f, &h->version);
	readUInt32le(f, &h->unknown1);
	readInt32le(f, &h->max_image_records);
	readInt32le(f, &h->num_image_records);
	readInt32le(f, &h->num_bitmap_records);
	readInt32le(f, &h->num_bitmap_records_without_system);
	readUInt32le(f, &h->total_filesize);
	readUInt32le(f, &h->filesize_555);
	readUInt32le(f, &h->filesize_external);
	fseek(f, SG_HEADER_SIZE, SEEK_SET);
	return h;
}

SgFile::SgFile(const char *filename)
	: bitmaps(NULL), bitmaps_n(0),
	  images(NULL), images_n(0),
	  header(NULL)
{
	this->filename = strdup(filename);
}

SgFile::~SgFile() {
	for (int i = 0; i < bitmaps_n; i++) {
		sg_delete_bitmap(bitmaps[i]);
		bitmaps[i] = NULL;
	}
	free(bitmaps);
	for (int i = 0; i < images_n; i++) {
		sg_delete_image(images[i]);
		images[i] = NULL;
	}
	free(images);
	free(filename);
	if (header != NULL)
		free(header);
}

int SgFile::bitmapCount() const {
	return bitmaps_n;
}

int SgFile::imageCount(int bitmapId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps_n) {
		return -1;
	}
	
	return sg_get_bitmap_image_count(bitmaps[bitmapId]);
}

int SgFile::totalImageCount() const {
	return images_n;
}

SgImage *SgFile::image(int imageId) const {
	if (imageId < 0 || imageId >= images_n) {
		return NULL;
	}
	return images[imageId];
}

SgImage *SgFile::image(int bitmapId, int imageId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps_n ||
	    imageId < 0 || imageId >= sg_get_bitmap_image_count(bitmaps[bitmapId])) {
		return NULL;
	}
	
	return sg_get_bitmap_image(bitmaps[bitmapId], imageId);
}

SgBitmap *SgFile::getBitmap(int bitmapId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps_n) {
		return NULL;
	}
	
	return bitmaps[bitmapId];
}

bool SgFile::load() {
	FILE *file = fopen(filename, "r");

	if (file == NULL) {
		printf("unable to open file\n");
		return false;
	}

	header = sg_read_header(file);
	
	if (!checkVersion()) {
		fclose(file);
		return false;
	}
	
	printf("Read header, num bitmaps = %d, num images = %d",
               header->num_bitmap_records, header->num_image_records);
	
	loadBitmaps(file);
	
	int pos = SG_HEADER_SIZE + maxBitmapRecords() * SG_BITMAP_RECORD_SIZE;
	fseek(file, pos, SEEK_SET);
	
	loadImages(file, header->version >= 0xd6);
	fclose(file);
	
	if (bitmaps_n > 1 && images_n == sg_get_bitmap_image_count(bitmaps[0])) {
		printf("SG file has %d bitmaps but only the first is in use\n",
			bitmaps_n);
		// Remove the bitmaps other than the first
		for (int i = bitmaps_n - 1; i > 0; i--) {
			SgBitmap *bmp = bitmaps[i];
			sg_delete_bitmap(bmp);
		}
		bitmaps_n = 1;
	}
	
	return true;
}

void SgFile::loadBitmaps(FILE *file) {
	if (bitmaps != NULL) {
		free(bitmaps);
		bitmaps_n = 0;
	}
	bitmaps_n = header->num_bitmap_records;
	bitmaps = (SgBitmap**)(malloc(sizeof(SgBitmap*) * bitmaps_n));

	for (int i = 0; i < header->num_bitmap_records; i++) {
		SgBitmap *bitmap = sg_read_bitmap(i, filename, file);
		bitmaps[i] = bitmap;
	}
}

void SgFile::loadImages(FILE *file, bool includeAlpha) {
	if (images != NULL) {
		free(images);
		images_n = 0;
	}
	images_n = header->num_image_records;
	images = (SgImage**)(malloc(sizeof(SgImage*) * images_n));

	// The first one is a dummy/null record
	sg_read_image(0, file, includeAlpha);
	
	for (int i = 0; i < header->num_image_records; i++) {
		struct SgImage *image = sg_read_image(i + 1, file, includeAlpha);
		int32_t invertOffset = sg_get_image_invert_offset(image);
		if (invertOffset < 0 && (i + invertOffset) >= 0) {
			sg_set_image_invert(image, images[i + invertOffset]);
		}
		int bitmapId = sg_get_image_bitmapid(image);
		if (bitmapId >= 0 && bitmapId < bitmaps_n) {
			sg_add_bitmap_image(bitmaps[bitmapId], image);
			sg_set_image_parent(image, bitmaps[bitmapId]);
		} else {
			printf("Image %d has no parent: %d\n", i, bitmapId);
		}
		
		images[i] = image;
	}
}

bool SgFile::checkVersion() {
	if (header->version == 0xd3) {
		// SG2 file: filesize = 74480 or 522680 (depending on whether it's
		// a "normal" sg2 or an enemy sg2
		if (header->sg_filesize == 74480 || header->sg_filesize == 522680) {
			return true;
		}
	} else if (header->version == 0xd5 || header->version == 0xd6) {
		// SG3 file: filesize = the actual size of the sg3 file
		FILE *fp = fopen(filename, "r");
		fseek(fp, 0, SEEK_END);
		uint32_t filesize = uint32_t(ftell(fp));

		if (header->sg_filesize == 74480 || filesize == header->sg_filesize) {
			return true;
		}
	}
	
	// All other cases:
	return false;
}

int SgFile::maxBitmapRecords() const {
	if (header->version == 0xd3) {
		return 100; // SG2
	} else {
		return 200; // SG3
	}
}
