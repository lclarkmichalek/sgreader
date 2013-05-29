#include "sgfile.h"
#include "sgimage.h"
#include "utils.h"

#include <QDataStream>
#include <QFile>
#include <QFileInfo>

enum {
	SG_HEADER_SIZE = 680
};

class SgHeader {
public:
	SgHeader(FILE *f) {
		readUInt32le(f, &sg_filesize);
		readUInt32le(f, &version);
		readUInt32le(f, &unknown1);
		readInt32le(f, &max_image_records);
		readInt32le(f, &num_image_records);
		readInt32le(f, &num_bitmap_records);
		readInt32le(f, &num_bitmap_records_without_system);
		readUInt32le(f, &total_filesize);
		readUInt32le(f, &filesize_555);
		readUInt32le(f, &filesize_external);
		fseek(f, SG_HEADER_SIZE, SEEK_SET);
	}
	
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

SgFile::SgFile(const QString &filename)
	: header(NULL)
{
	this->filename = filename;
	QFileInfo fi(filename);
	this->basefilename = fi.baseName();
}

SgFile::~SgFile() {
	for (int i = 0; i < bitmaps.size(); i++) {
		delete bitmaps[i];
		bitmaps[i] = 0;
	}
	for (int i = 0; i < images.size(); i++) {
		delete images[i];
		images[i] = 0;
	}
}

int SgFile::bitmapCount() const {
	return bitmaps.size();
}

int SgFile::imageCount(int bitmapId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps.size()) {
		return -1;
	}
	
	return bitmaps[bitmapId]->imageCount();
}

QString SgFile::basename() const {
	return basefilename;
}

int SgFile::totalImageCount() const {
	return images.size();
}

SgImage *SgFile::image(int imageId) const {
	if (imageId < 0 || imageId >= images.size()) {
		return NULL;
	}
	return images[imageId];
}

SgImage *SgFile::image(int bitmapId, int imageId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps.size() ||
		imageId < 0 || imageId >= bitmaps[bitmapId]->imageCount()) {
		return NULL;
	}
	
	return bitmaps[bitmapId]->image(imageId);
}

QImage SgFile::getImage(int imageId) {
	if (imageId < 0 || imageId >= images.size()) {
		qDebug("Id out of range");
		return QImage();
	}
	return images[imageId]->getImage();
}

QImage SgFile::getImage(int bitmapId, int imageId) {
	if (bitmapId < 0 || bitmapId >= bitmaps.size() ||
		imageId < 0 || imageId >= bitmaps[bitmapId]->imageCount()) {
		qDebug("Id out of range");
		return QImage();
	}
	
	return bitmaps[bitmapId]->getImage(imageId);
}

SgBitmap *SgFile::getBitmap(int bitmapId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps.size()) {
		return NULL;
	}
	
	return bitmaps[bitmapId];
}

QString SgFile::getBitmapDescription(int bitmapId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps.size()) {
		return QString();
	}
	
	return bitmaps[bitmapId]->description();
}

QString SgFile::errorMessage(int bitmapId, int imageId) const {
	if (bitmapId < 0 || bitmapId >= bitmaps.size() ||
		imageId < 0 || imageId >= bitmaps[bitmapId]->imageCount()) {
		return QString();
	}
	
	return bitmaps[bitmapId]->errorMessage(imageId);
}

QString SgFile::errorMessage(int imageId) const {
	if (imageId < 0 || imageId >= images.size()) {
		return QString();
	}
	return images[imageId]->errorMessage();
}

bool SgFile::load() {
	FILE *file = fopen(filename.toStdString().c_str(), "r");

	if (file == NULL) {
		qDebug("unable to open file");
		return false;
	}

	header = new SgHeader(file);
	
	if (!checkVersion()) {
		fclose(file);
		return false;
	}
	
	qDebug("Read header, num bitmaps = %d, num images = %d",
		header->num_bitmap_records, header->num_image_records);
	
	loadBitmaps(file);
	
	int pos = SG_HEADER_SIZE + maxBitmapRecords() * SgBitmap::RECORD_SIZE;
	fseek(file, pos, SEEK_SET);
	
	loadImages(file, header->version >= 0xd6);
	fclose(file);
	
	if (bitmaps.size() > 1 && images.size() == bitmaps[0]->imageCount()) {
		qDebug("SG file has %d bitmaps but only the first is in use",
			bitmaps.size());
		// Remove the bitmaps other than the first
		for (int i = bitmaps.size() - 1; i > 0; i--) {
			SgBitmap *bmp = bitmaps.takeLast();
			delete bmp;
		}
	}
	
	return true;
}

void SgFile::loadBitmaps(FILE *file) {
	
	for (int i = 0; i < header->num_bitmap_records; i++) {
		SgBitmap *bitmap = new SgBitmap(i, filename, file);
		bitmaps.append(bitmap);
	}
	
}

void SgFile::loadImages(FILE *file, bool includeAlpha) {
	// The first one is a dummy/null record
	SgImage dummy(0, file, includeAlpha);
	
	for (int i = 0; i < header->num_image_records; i++) {
		SgImage *image = new SgImage(i + 1, file, includeAlpha);
		qint32 invertOffset = image->invertOffset();
		if (invertOffset < 0 && (i + invertOffset) >= 0) {
			image->setInvertImage(images[i + invertOffset]);
		}
		int bitmapId = image->bitmapId();
		if (bitmapId >= 0 && bitmapId < bitmaps.size()) {
			bitmaps[bitmapId]->addImage(image);
			image->setParent(bitmaps[bitmapId]);
		} else {
			qDebug("Image %d has no parent: %d", i, bitmapId);
		}
		images.append(image);
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
		QFileInfo fi(filename);
		if (header->sg_filesize == 74480 || fi.size() == header->sg_filesize) {
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
