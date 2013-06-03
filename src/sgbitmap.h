#ifndef SGBITMAP_H
#define SGBITMAP_H

#include <stdbool.h>
#include <stdio.h>

class SgBitmapRecord;
class SgImage;

class SgBitmap {
	public:
		SgBitmap(int id, const char *sgFilename, FILE *file);
		~SgBitmap();
		int imageCount() const;
                char *description() const;
                char *bitmapName() const;
		SgImage *image(int id);
		void addImage(SgImage *child);
                const char *getFilename() const;
                bool isExtern() const;
		
		enum {
			RECORD_SIZE = 200
		};
	private:
		SgImage **images;
		int images_n;
		int images_c;
		SgBitmapRecord *record;
		char *sgFilename;
		int bitmapId;
};

#endif /* SGBITMAP_H */
