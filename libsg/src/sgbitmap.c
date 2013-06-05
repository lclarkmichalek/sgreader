#include <sg/sgbitmap.h>
#include <sg/sgimage.h>
#include <sg/utils.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

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

struct SgBitmap {
    struct SgImage **images;
    int images_n;
    int images_c;
    struct SgBitmapRecord *record;
    char *sgFilename;
    int bitmapId;
};


struct SgBitmapRecord *sg_read_bitmap_record(FILE *f) {
	struct SgBitmapRecord *rec = (struct SgBitmapRecord*)malloc(sizeof(struct SgBitmapRecord));
	fread(&rec->filename, 65, 1, f);
	rec->filename[64] = 0;
	fread(&rec->comment, 51, 1, f);
	rec->comment[50] = 0;
	
	readUInt32le(f, &rec->width);
	readUInt32le(f, &rec->height);
	readUInt32le(f, &rec->num_images);
	readUInt32le(f, &rec->start_index);
	readUInt32le(f, &rec->end_index);
	
	fseek(f, 64, SEEK_CUR);
        
        return rec;
}

struct SgBitmap *sg_read_bitmap(int id, const char *sgFilename, FILE *file) {
	struct SgBitmap *bmp = (struct SgBitmap*)malloc(sizeof(struct SgBitmap));

	bmp->images = NULL;
	bmp->images_n = 0;
	bmp->images_c = 0;

	bmp->bitmapId = id;
	bmp->sgFilename = strdup(sgFilename);
	bmp->record = sg_read_bitmap_record(file);

	return bmp;
}

void sg_delete_bitmap(struct SgBitmap *bmp) {
	free(bmp->sgFilename);
	if (bmp->images != NULL)
		free(bmp->images);
	free(bmp);
}

const char *sg_get_bitmap_filename(struct SgBitmap *bmp) {
    return (const char*)bmp->record->filename;
}

const char *sg_get_bitmap_comment(struct SgBitmap *bmp) {
    return (const char*)bmp->record->comment;
}

uint32_t sg_get_bitmap_width(struct SgBitmap *bmp) {
    return bmp->record->width;
}

uint32_t sg_get_bitmap_height(struct SgBitmap *bmp) {
    return bmp->record->height;
}

uint32_t sg_get_bitmap_num_images(struct SgBitmap *bmp) {
    return bmp->record->num_images;
}

uint32_t sg_get_bitmap_start_index(struct SgBitmap *bmp) {
    return bmp->record->start_index;
}

uint32_t sg_get_bitmap_end_index(struct SgBitmap *bmp) {
    return bmp->record->end_index;
}

const char *sg_get_bitmap_sg_filename(struct SgBitmap *bmp) {
    return (const char*)bmp->sgFilename;
}

int sg_get_bitmap_id(struct SgBitmap *bmp) {
    return bmp->bitmapId;
}

struct SgImage *sg_get_bitmap_image(struct SgBitmap *bmp, int i) {
    if (i < 0 || i >= bmp->images_n) {
        return NULL;
    }
    return bmp->images[i];
}

int sg_get_bitmap_image_count(struct SgBitmap *bmp) {
    return bmp->images_n;
}

void sg_add_bitmap_image(struct SgBitmap *bmp, struct SgImage *child) {
	if (bmp->images_n >= bmp->images_c) {
		int new_cap = bmp->images_c * 2 | 4;
		struct SgImage **new_arr = (struct SgImage**)(malloc(new_cap * sizeof(struct SgImage*)));
		int i;
		for (i = 0; i < bmp->images_c; i++) {
			new_arr[i] = bmp->images[i];
		}
		free(bmp->images);
		bmp->images = new_arr;
		bmp->images_c = new_cap;
	}
	bmp->images[bmp->images_n++] = child;
}
