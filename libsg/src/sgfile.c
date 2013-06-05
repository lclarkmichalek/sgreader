#include <sg/sgfile.h>
#include <sg/sgimage.h>
#include <sg/utils.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

enum {
	SG_HEADER_SIZE = 680,
	SG_BITMAP_RECORD_SIZE = 200
};

struct SgHeader {
	uint32_t sg_filesize;
	uint32_t version;
	uint32_t unknown1;
	int32_t max_image_records;
	int32_t num_image_records;
	int32_t num_bitmap_records;
	int32_t num_bitmap_records_without_system; /* ? */
	uint32_t total_filesize;
	uint32_t filesize_555;
	uint32_t filesize_external;
};

struct SgFile {
	struct SgBitmap **bitmaps;
	int bitmaps_n;
	struct SgImage **images;
	int images_n;
	char* filename;
	struct SgHeader *header;
};

bool checkVersion(struct SgFile *sgf);
void loadBitmaps(struct SgFile *sgf, FILE *file);
void loadImages(struct SgFile *sgf, FILE *file, bool hasAlpha);
int maxBitmapRecords(struct SgFile *sgf);

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

struct SgFile *sg_read_file(const char *filename) {
	struct SgFile *sgf = (struct SgFile*)malloc(sizeof(struct SgFile));
	sgf->filename = strdup(filename);
	sgf->bitmaps = NULL;
	sgf->bitmaps_n = 0;
	sgf->images = NULL;
	sgf->images_n = 0;

	FILE *file = fopen(filename, "r");

	if (file == NULL) {
		printf("Unable to open file\n");
		return NULL;
	}

	sgf->header = sg_read_header(file);

	if (!checkVersion(sgf)) {
		fclose(file);
		return NULL;
	}

	printf("Read header, num bitmaps = %d, num images = %d",
               sgf->header->num_bitmap_records, sgf->header->num_image_records);

	loadBitmaps(sgf, file);
	
	int pos = SG_HEADER_SIZE + maxBitmapRecords(sgf) * SG_BITMAP_RECORD_SIZE;
	fseek(file, pos, SEEK_SET);
	
	loadImages(sgf, file, sgf->header->version >= 0xd6);
	fclose(file);
	
	if (sgf->bitmaps_n > 1 && sgf->images_n == sg_get_bitmap_image_count(sgf->bitmaps[0])) {
		printf("SG file has %d bitmaps but only the first is in use\n",
		       sgf->bitmaps_n);
		// Remove the bitmaps other than the first
		int i;
		for (i = sgf->bitmaps_n - 1; i > 0; i--) {
			struct SgBitmap *bmp = sgf->bitmaps[i];
			sg_delete_bitmap(bmp);
		}
		sgf->bitmaps_n = 1;
	}
	
	return sgf;
}

void sg_delete_file(struct SgFile *file) {
	int i;
	for (i = 0; i < file->bitmaps_n; i++) {
		sg_delete_bitmap(file->bitmaps[i]);
	}
	free(file->bitmaps);
	for (i = 0; i < file->images_n; i++) {
		sg_delete_image(file->images[i]);
	}
	free(file->images);
	free(file->filename);
	free(file->header);
}

void loadBitmaps(struct SgFile *sgf, FILE *file) {
	if (sgf->bitmaps != NULL) {
		free(sgf->bitmaps);
		sgf->bitmaps_n = 0;
	}
	sgf->bitmaps_n = sgf->header->num_bitmap_records;
	sgf->bitmaps = (struct SgBitmap**)(malloc(sizeof(struct SgBitmap*) * sgf->bitmaps_n));

	int i;
	for (i = 0; i < sgf->header->num_bitmap_records; i++) {
		struct SgBitmap *bitmap = sg_read_bitmap(i, sgf->filename, file);
		sgf->bitmaps[i] = bitmap;
	}
}

void loadImages(struct SgFile *sgf, FILE *file, bool includeAlpha) {
	if (sgf->images != NULL) {
		free(sgf->images);
		sgf->images_n = 0;
	}
	sgf->images_n = sgf->header->num_image_records;
	sgf->images = (struct SgImage**)(malloc(sizeof(struct SgImage*) * sgf->images_n));

	// The first one is a dummy/null record
	sg_read_image(0, file, includeAlpha);
	
	int i;
	for (i = 0; i < sgf->header->num_image_records; i++) {
		struct SgImage *image = sg_read_image(i + 1, file, includeAlpha);
		int32_t invertOffset = sg_get_image_invert_offset(image);
		if (invertOffset < 0 && (i + invertOffset) >= 0) {
			sg_set_image_invert(image, sgf->images[i + invertOffset]);
		}
		int bitmapId = sg_get_image_bitmapid(image);
		if (bitmapId >= 0 && bitmapId < sgf->bitmaps_n) {
			sg_add_bitmap_image(sgf->bitmaps[bitmapId], image);
			sg_set_image_parent(image, sgf->bitmaps[bitmapId]);
		} else {
			printf("Image %d has no parent: %d\n", i, bitmapId);
		}
		
		sgf->images[i] = image;
	}
}

bool checkVersion(struct SgFile *file) {
	if (file->header->version == 0xd3) {
		// SG2 file: filesize = 74480 or 522680 (depending on whether it's
		// a "normal" sg2 or an enemy sg2
		if (file->header->sg_filesize == 74480 || file->header->sg_filesize == 522680) {
			return true;
		}
	} else if (file->header->version == 0xd5 || file->header->version == 0xd6) {
		// SG3 file: filesize = the actual size of the sg3 file
		FILE *fp = fopen(file->filename, "r");
		fseek(fp, 0, SEEK_END);
		uint32_t filesize = (uint32_t)ftell(fp);

		if (file->header->sg_filesize == 74480 || filesize == file->header->sg_filesize) {
			return true;
		}
	}
	
	// All other cases:
	return false;
}

int maxBitmapRecords(struct SgFile *file) {
	if (file->header->version == 0xd3) {
		return 100; // SG2
	} else {
		return 200; // SG3
	}
}

uint32_t sg_get_file_version(struct SgFile *file) {
	return file->header->version;
}

uint32_t sg_get_file_total_filesize(struct SgFile *file) {
	return file->header->total_filesize;
}

uint32_t sg_get_file_555_filesize(struct SgFile *file) {
	return file->header->filesize_555;
}

uint32_t sg_get_file_external_filesize(struct SgFile *file) {
	return file->header->filesize_external;
}

int sg_get_file_bitmap_count(struct SgFile *file) {
	return file->bitmaps_n;
}
struct SgBitmap *sg_get_file_bitmap(struct SgFile *file, int i) {
	if (i < 0 || i >= file->bitmaps_n)
		return NULL;

	return file->bitmaps[i];
}

int sg_get_file_image_count(struct SgFile *file) {
	return file->images_n;
}
struct SgImage *sg_get_file_image(struct SgFile *file, int i) {
	if (i < 0 || i >= file->images_n)
		return NULL;

	return file->images[i];
}
