#ifndef SGFILE_H
#define SGFILE_H

#include <stdio.h>

class SgBitmap;
class SgImage;
class SgHeader; // = header

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
		char* basefilename;
		SgHeader *header;
};

#endif /* SGFILE_H */
