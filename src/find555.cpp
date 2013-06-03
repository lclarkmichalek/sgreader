#include "./find555.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>

#include <stdbool.h>

QString findFilenameCaseInsensitive(QDir directory, QString filename);
QString find555File(struct SgBitmap *bitmap);

QString find555Filename(struct SgBitmap *bitmap) {
	QString filename = find555File(bitmap);
	if (filename.isEmpty()) {
		return QString();
	}

	QFile *file = new QFile(filename);
	if (!file->open(QIODevice::ReadOnly)) {
		filename = QString();
	}
	delete file;
	return filename;
}

QString find555File(SgBitmap *bitmap) {
	QFileInfo fileinfo(bitmap->sgFilename);
	bool isExtern = is_sg_bitmap_extern(bitmap);
	
	// Fetch basename of the file
	// either the same name as sg(2|3) or from file record
	QString basename;
	if (isExtern) {
		basename = QString(bitmap->sgFilename);
	} else {
		QFileInfo fileinfo(bitmap->sgFilename);
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

QString findFilenameCaseInsensitive(QDir directory, QString filename) {
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
