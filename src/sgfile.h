#ifndef SGFILE_H
#define SGFILE_H

#include <stdio.h>

#include <QString>
#include <QList>
#include <QImage>

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
		char* basename() const;
		
		SgBitmap *getBitmap(int bitmapId) const;
		QString getBitmapDescription(int bitmapId) const;
		
		SgImage *image(int bitmapId, int imageId) const;
		SgImage *image(int globalImageId) const;
		
		QImage getImage(int bitmapId, int imageId);
		QImage getImage(int globalImageId);
		
		QString errorMessage(int bitmapId, int imageId) const;
		QString errorMessage(int globalImageId) const;
		
	private:
		bool checkVersion();
		int maxBitmapRecords() const;
		void loadBitmaps(FILE *stream);
		void loadImages(FILE *stream, bool includeAlpha);
		
		QList<SgBitmap*> bitmaps;
		QList<SgImage*> images;
		char* filename;
		char* basefilename;
		SgHeader *header;
};

#endif /* SGFILE_H */
