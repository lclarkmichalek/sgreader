#include "extractthread.h"

#include "../sgfile.h"
#include "../sgbitmap.h"
#include "../sgimage.h"
#include "../find555.h"

#include <QDebug>
#include <QFileInfo>
#include <QImage>

ExtractThread::ExtractThread(const QStringList &files, const QString &outputDir,
	bool extractSystem)
	: files(files), extractSystem(extractSystem), doCancel(false)
{
	this->outputDir.setPath(outputDir);
	qDebug() << "Creating thread." << files << outputDir << extractSystem;
}

void ExtractThread::cancel() {
	doCancel = true;
}

QStringList ExtractThread::errors() const {
	return errorMessages;
}

int ExtractThread::errorCount() const {
	return errorImages;
}

int ExtractThread::extractCount() const {
	return extracted;
}

void ExtractThread::run() {
	int numfiles = files.size();
	extracted = 0;
	errorImages = 0;
	
	outputDir.mkpath(outputDir.path());
	
	for (int i = 0; i < numfiles; i++) {
		extractFile(files[i]);
		
		if (doCancel) {
			errorMessages.append("*** Extraction cancelled by user ***");
			break;
		}
	}
}

void ExtractThread::extractFile(const QString &filename) {
	SgFile sg(filename.toStdString().c_str());
	int numImages, bitmaps, i = 0;
	
	qDebug() << "Extracting file" << filename;
	
	sg.load();
	QFileInfo fi(filename);
	QString basename = fi.baseName();
	outputDir.mkdir(basename);
	outputDir.cd(basename);
	
	bitmaps = sg.bitmapCount();
	numImages = sg.totalImageCount();
	qDebug("Bitmaps: %d", bitmaps);
	
	if (!extractSystem && bitmaps > 1) {
		numImages -= sg.imageCount(0);
		i++;
	}
	emit fileChanged(basename, numImages);
	QString bmpName = basename;
	
	int total = 0;
	for (; i < bitmaps; i++) {
		SgBitmap *bitmap = sg.getBitmap(i);
		if (bitmaps != 1) {
			bmpName = QString(bitmap->record->filename).remove(".bmp", Qt::CaseInsensitive);
		}
		int images = bitmap->images_n;

                QString filename = find555Filename(bitmap);
		
		for (int n = 0; n < images; n++) {
			emit progressChanged(++total);
			
			struct SgImageData *sgData = 
                            get_sg_image_data(bitmap->images[n], filename.toStdString().c_str());
                        QImage img((uchar*)(sgData->data),
                                   sgData->width, sgData->height,
                                   QImage::Format_ARGB32);
			if (!img.isNull()) {
				QString pngfile = QString("%0_%1.png")
					.arg(bmpName)
					.arg(n + 1, 5, 10, QChar('0'));
				
				img.save(outputDir.filePath(pngfile));
				extracted++;
			} else {
				QString error;
				if (bitmaps == 1) {
					error = QString("File '%0', image %1: %2")
						.arg(basename)
						.arg(n + 1)
						.arg(bitmap->images[n]->error);
				} else {
					error = QString("File '%0', section '%1', image %2: %3")
						.arg(basename)
						.arg(bmpName)
						.arg(n + 1)
						.arg(bitmap->images[n]->error);
				}
				errorMessages.append(error);
				errorImages++;
			}
                        delete_sg_image_data(sgData);
			
			if (doCancel) return;
		}
	}
	outputDir.cd("..");
	qDebug("done");
}
