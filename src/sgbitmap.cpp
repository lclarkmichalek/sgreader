#include "sgbitmap.h"
#include "sgimage.h"
#include "utils.h"

#include <strings.h>

#include <QFile>

class SgBitmapRecord {
public:
	SgBitmapRecord(FILE *f) {
		fread(&filename, 65, 1, f);
		filename[64] = 0;
		fread(&comment, 51, 1, f);
		comment[50] = 0;
		
		readUInt32le(f, &width);
		readUInt32le(f, &height);
		readUInt32le(f, &num_images);
		readUInt32le(f, &start_index);
		readUInt32le(f, &end_index);
		
		fseek(f, 64, SEEK_CUR);
	}
	
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

SgBitmap::SgBitmap(int id, const char *sgFilename, FILE *file)
	: images(NULL), images_n(0), images_c(0),
	  file(NULL)
{
	bitmapId = id;
	this->sgFilename = strdup(sgFilename);
	record = new SgBitmapRecord(file);
}

SgBitmap::~SgBitmap() {
	if (file) {
		delete file;
	}
	free(sgFilename);
	if (images != NULL)
		free(images);
}

int SgBitmap::imageCount() const {
	return images_n;
}

QString SgBitmap::description() const {
	return QString("%0 (%1)")
		.arg(record->filename)
		.arg(images_n);
}

QString SgBitmap::bitmapName() const {
	return QString(record->filename).remove(".bmp", Qt::CaseInsensitive);
}

void SgBitmap::addImage(SgImage *child) {
	if (images_n >= images_c) {
		int new_cap = images_c * 2 | 4;
		SgImage **new_arr = (SgImage**)(malloc(new_cap * sizeof(SgImage*)));
		int i;
		for (i = 0; i < images_c; i++) {
			new_arr[i] = images[i];
		}
		free(images);
		images = new_arr;
		images_c = new_cap;
	}
	images[images_n++] = child;
}

SgImage *SgBitmap::image(int id) {
	if (id < 0 || id >= images_n) {
		return NULL;
	}
	
	return images[id];
}

QFile *SgBitmap::openFile(char isExtern) {
	if (file && this->isExtern != isExtern) {
		delete file;
		file = NULL;
	}
	this->isExtern = isExtern;
	if (!file) {
		QString filename = find555File();
		if (filename.isEmpty()) {
			return NULL;
		}
		
		file = new QFile(filename);
		if (!file->open(QIODevice::ReadOnly)) {
			file = NULL;
		}
	}
	return file;
}

QString SgBitmap::find555File() {
	QFileInfo fileinfo(sgFilename);
	
	// Fetch basename of the file
	// either the same name as sg(2|3) or from file record
	QString basename;
	if (isExtern) {
		basename = QString(record->filename);
	} else {
		QFileInfo fileinfo(sgFilename);
		basename = fileinfo.fileName();
	}
	
	// Change the extension to .555
	int position = basename.lastIndexOf('.');
	if (position != -1) {
		basename.replace(position + 1, 3, "555");
	}
	//qDebug() << "Searching for file: " << basename;
	
	QString path = findFilenameCaseInsensitive(fileinfo.dir(), basename);
	if (path.length() > 0) {
		return path;
	}
	
	QDir dirinfo = fileinfo.dir();
	if (dirinfo.cd("555")) {
		return findFilenameCaseInsensitive(dirinfo, basename);
	}
	return QString();
}

QString SgBitmap::findFilenameCaseInsensitive(QDir directory, QString filename) {
	filename = filename.toLower();
	
	QStringList files = directory.entryList(QStringList(filename), QDir::Files);
	for (int i = 0; i < files.size(); i++) {
		if (filename == files[i].toLower()) {
			return directory.absoluteFilePath(files[i]);
		}
		//qDebug() << "No match: " << files[i];
	}
	
	return QString();
}

