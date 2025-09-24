#include "TACFolderWidget.h"
#include <QPushButton>
#include "common.h"
TACFolderWidget::TACFolderWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACFolderWidget");
	QPushButton* button = new QPushButton(this);
	button->setObjectName("homeButton");
	button->setFixedSize(QSize(80,80));
	button->setIconSize(QSize(60, 60));
	button->move(5, 5);
	connect(button, &QPushButton::clicked, this, [=]() {
		emit clicked();
	});
	this->setRadius(50);
	this->setFixedSize(QSize(100, 100));
}

TACFolderWidget::~TACFolderWidget()
{}
void TACFolderWidget::initShow()
{
	QRect rect = this->getScreenGeometryWithTaskbar();
	if (rect.isEmpty())
		return;
	QSize windowSize = this->size();
	int x = 30;
	int y = rect.height()-windowSize.height()-60;
	this->move(x, y);
}
