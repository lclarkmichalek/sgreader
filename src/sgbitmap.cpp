#include "sgbitmap.h"
#include "sgimage.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

struct SgBitmapRecord *read_sg_bitmap_record(FILE *f) {
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

struct SgBitmap *read_sg_bitmap(int id, const char *sgFilename, FILE *file) {
	struct SgBitmap *bmp = (struct SgBitmap*)malloc(sizeof(struct SgBitmap));

	bmp->images = NULL;
	bmp->images_n = 0;
	bmp->images_c = 0;

	bmp->bitmapId = id;
	bmp->sgFilename = strdup(sgFilename);
	bmp->record = read_sg_bitmap_record(file);

	return bmp;
}

void delete_sg_bitmap(struct SgBitmap *bmp) {
	free(bmp->sgFilename);
	if (bmp->images != NULL)
		free(bmp->images);
	free(bmp);
}

void add_sg_bitmap_image(struct SgBitmap *bmp, struct SgImage *child) {
	if (bmp->images_n >= bmp->images_c) {
		int new_cap = bmp->images_c * 2 | 4;
		SgImage **new_arr = (SgImage**)(malloc(new_cap * sizeof(SgImage*)));
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

bool is_sg_bitmap_extern(struct SgBitmap *bmp) {
    if (bmp->images_n == 0) {
        return false;
    }
    return is_sg_image_extern(bmp->images[0]);
}
