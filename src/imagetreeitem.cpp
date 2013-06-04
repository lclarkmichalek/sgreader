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
                .arg(sg_get_image_width(imageRecord))
                .arg(sg_get_image_height(imageRecord)));
	//setData(0, Qt::DisplayRole, imageId);
	//setData(1, Qt::DisplayRole, imageRecord->description());
        setToolTip(0, QString("ID %7: offset %0, length %1, width %2, height %3, type %5, %6")
		   .arg(sg_get_image_offset(imageRecord))
		   .arg(sg_get_image_length(imageRecord))
		   .arg(sg_get_image_width(imageRecord))
		   .arg(sg_get_image_height(imageRecord))
		   .arg(sg_get_image_type(imageRecord))
		   .arg(sg_get_image_extern(imageRecord) ? "external" : "internal")
		   .arg(imageId));

	//setToolTip(0, imageRecord->fullDescription());
}
