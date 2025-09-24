#pragma execution_character_set("utf-8")
#include "TACDesktopManagerWidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "common.h"
TACDesktopManagerWidget::TACDesktopManagerWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
	this->setObjectName("TACDesktopManagerWidget");

	this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	this->setBorderColor(WIDGET_BORDER_COLOR);
	this->setBorderWidth(WIDGET_BORDER_WIDTH);
	this->setRadius(15);
	this->visibleCloseButton(false);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(5);

	QLabel* label = new QLabel("桌面管理", this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

	QPushButton* fileManagerButton = new QPushButton("文件管理", this);
	layout->addWidget(fileManagerButton);

	QPushButton* createFolderButton = new QPushButton("创建文件夹", this);
	layout->addWidget(createFolderButton);

	QPushButton* countdowButton = new QPushButton("倒计时对话框", this);
	layout->addWidget(countdowButton);

	QPushButton* timeButton = new QPushButton("时间对话框", this);
	layout->addWidget(timeButton);

	QPushButton* classButton = new QPushButton("学校/班级", this);
	layout->addWidget(classButton);

	QPushButton* wallpaperButton = new QPushButton("壁纸", this);
	layout->addWidget(wallpaperButton);

	QPushButton* courseScheduleButton = new QPushButton("教师课程表", this);
	layout->addWidget(courseScheduleButton);

	QPushButton* tabelButton = new QPushButton("表格对话框", this);
	layout->addWidget(tabelButton);

	QPushButton* textButton = new QPushButton("文本对话框", this);
	layout->addWidget(textButton);

	QPushButton* imageButton = new QPushButton("图片对话框", this);
	layout->addWidget(imageButton);

	setLayout(layout);
	resize(230, 780);

}

TACDesktopManagerWidget::~TACDesktopManagerWidget()
{}
void TACDesktopManagerWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	initShow();
}

void TACDesktopManagerWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	initShow();
}
void TACDesktopManagerWidget::initShow()
{
}