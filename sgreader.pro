VPATH += ./src ./libsg/src ./libsg/sg

INCLUDEPATH += ./libsg
TEMPLATE = app
TARGET = 
DEPENDPATH += . gui
INCLUDEPATH += . gui
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
LIBS += -lsg

CONFIG += debug


# Input
HEADERS += aboutdialog.h \
           imagetreeitem.h \
           licencedialog.h \
           mainwindow.h \
           find555.h \
           gui/filelistpage.h \
           gui/inputdirpage.h \
           gui/outputdirpage.h \
           gui/progresspage.h \
           gui/extractthread.h \
           gui/extractwizard.h
SOURCES += aboutdialog.cpp \
           imagetreeitem.cpp \
           licencedialog.cpp \
           main.cpp \
           mainwindow.cpp \
           find555.cpp \
           gui/filelistpage.cpp \
           gui/inputdirpage.cpp \
           gui/outputdirpage.cpp \
           gui/progresspage.cpp \
           gui/extractthread.cpp \
           gui/extractwizard.cpp
RESOURCES += sgreader.qrc
RC_FILE = sgreader.rc
