#ifndef SGFILE_H
#define SGFILE_H

#include <stdio.h>

#include "./sgimage.h"
#include "./sgbitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

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

struct SgHeader *read_sg_header(FILE *file);

class SgFile {
	public:
		SgFile(const char* filename);
		~SgFile();
		bool load();
		int bitmapCount() const;
		int totalImageCount() const;
		int imageCount(int bitmapId) const;
		
		SgBitmap *getBitmap(int bitmapId) const;
		
		SgImage *image(int bitmapId, int imageId) const;
		SgImage *image(int globalImageId) const;
		
	private:
		bool checkVersion();
		int maxBitmapRecords() const;
		void loadBitmaps(FILE *stream);
		void loadImages(FILE *stream, bool includeAlpha);
		
		SgBitmap **bitmaps;
		int bitmaps_n;
		SgImage **images;
		int images_n;
		char* filename;
		SgHeader *header;
};

#ifdef __cplusplus
}
#endif

#endif /* SGFILE_H */
