#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui> // TODO: specify

#include <sg/sgfile.h>
#include <sg/sgimage.h>

class QAction;

class MainWindow : public QMainWindow {
	Q_OBJECT
	
	public:
		MainWindow();
		//~MainWindow();
		
	
	private slots:
		void openFile();
		void saveFile();
		void extractAll();
		void treeSelectionChanged();
		//void help();
		void licence();
		void about();
		
	private:
		void createChildren();
		void createMenu();
                void createActions();
		
		void loadFile(const QString &filename);
		void loadImage(SgImage *image);
		void clearImage();
		
		QTreeWidget *treeWidget;
		QLabel *imageLabel;
		QString filename;
		QString appname;
		QImage image;
                SgImageData *imageData;
		SgFile *sgFile;
		
		QAction *openAction;
		QAction *saveAction;
		QAction *extractAllAction;
		QAction *exitAction;
		
		QAction *helpAction;
		QAction *licenceAction;
		QAction *aboutAction;
};

#endif /* MAINWINDOW_H */
