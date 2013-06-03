#include "imagetreeitem.h"

ImageTreeItem::ImageTreeItem(QTreeWidget *parent, int id, SgImage *image)
	: QTreeWidgetItem(parent, ItemType), imageRecord(image), imageId(id)
{
	setColumnData();
}

ImageTreeItem::ImageTreeItem(QTreeWidgetItem *parent, int id, SgImage *image)
	: QTreeWidgetItem(parent, ItemType), imageRecord(image), imageId(id)
{
	setColumnData();
}

SgImage *ImageTreeItem::image() {
	return imageRecord;
}

void ImageTreeItem::setColumnData() {
	setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        setText(0, QString("%0: %1x%2").arg(imageId + 1)
                .arg(imageRecord->workRecord->width)
                .arg(imageRecord->workRecord->height));
	//setData(0, Qt::DisplayRole, imageId);
	//setData(1, Qt::DisplayRole, imageRecord->description());
        setToolTip(0, QString("ID %7: offset %0, length %1, width %2, height %3, type %5, %6")
		   .arg(imageRecord->workRecord->offset)
		   .arg(imageRecord->workRecord->length)
		   .arg(imageRecord->workRecord->width)
		   .arg(imageRecord->workRecord->height)
		   .arg(imageRecord->workRecord->type)
		   .arg(is_sg_image_extern(imageRecord) ? "external" : "internal")
		   .arg(imageId));

	//setToolTip(0, imageRecord->fullDescription());
}
