#pragma execution_character_set("utf-8")
#include "TACTrayWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "common.h"
TACTrayWidget::TACTrayWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACTrayWidget");

	this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	this->setBorderColor(WIDGET_BORDER_COLOR);
	this->setBorderWidth(WIDGET_BORDER_WIDTH);
	this->setRadius(15);
	this->visibleCloseButton(false);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(5);

	QLabel* label = new QLabel("", this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

	QPushButton* fileManagerButton = new QPushButton("配置学校信息", this);
	fileManagerButton->setCheckable(true);
	layout->addWidget(fileManagerButton);
	
	// 匿名函数（Lambda）作为点击事件
	connect(fileManagerButton, &QPushButton::toggled, this, [=](bool checked) {
		emit navChoolInfo(checked);
		});

	QPushButton* createFolderButton = new QPushButton("桌面组件", this);
	layout->addWidget(createFolderButton);
	createFolderButton->setCheckable(true);

	// 匿名函数（Lambda）作为点击事件
	connect(createFolderButton, &QPushButton::toggled, this, [=](bool checked) {
		emit navType(checked);
	});

	//QPushButton* countdowButton = new QPushButton("对讲", this);
	//layout->addWidget(countdowButton);

	//QPushButton* timeButton = new QPushButton("消息", this);
	//layout->addWidget(timeButton);

	setLayout(layout);
	resize(230, 150);

}

TACTrayWidget::~TACTrayWidget()
{}
void TACTrayWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	initShow();
}

void TACTrayWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	initShow();
}
void TACTrayWidget::initShow()
{
}