#ifndef SGBITMAP_H
#define SGBITMAP_H

#include <QImage>
#include <QList>
#include <QString>
#include <QDir>

class QDataStream;
class QFile;
class SgBitmapRecord;
class SgImage;

class SgBitmap {
	public:
		SgBitmap(int id, const char *sgFilename, FILE *file);
		~SgBitmap();
		int imageCount() const;
		QString description() const;
		QString bitmapName() const;
		SgImage *image(int id);
		void addImage(SgImage *child);
		QFile *openFile(char isExtern);
		
		enum {
			RECORD_SIZE = 200
		};
	private:
		QString find555File();
		QString findFilenameCaseInsensitive(QDir directory, QString filename);
		
		SgImage **images;
		int images_n;
		int images_c;
		SgBitmapRecord *record;
		QFile *file;
		char *sgFilename;
		int bitmapId;
		char isExtern;
};

#endif /* SGBITMAP_H */
