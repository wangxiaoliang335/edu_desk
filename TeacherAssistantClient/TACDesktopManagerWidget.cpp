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

	QLabel* label = new QLabel("�������", this);
	label->setAlignment(Qt::AlignCenter);
	layout->addWidget(label);

	QPushButton* fileManagerButton = new QPushButton("�ļ�����", this);
	layout->addWidget(fileManagerButton);

	QPushButton* createFolderButton = new QPushButton("�����ļ���", this);
	layout->addWidget(createFolderButton);

	QPushButton* countdowButton = new QPushButton("����ʱ�Ի���", this);
	layout->addWidget(countdowButton);

	QPushButton* timeButton = new QPushButton("ʱ��Ի���", this);
	layout->addWidget(timeButton);

	QPushButton* classButton = new QPushButton("ѧУ/�༶", this);
	layout->addWidget(classButton);

	QPushButton* wallpaperButton = new QPushButton("��ֽ", this);
	layout->addWidget(wallpaperButton);

	QPushButton* courseScheduleButton = new QPushButton("��ʦ�γ̱�", this);
	layout->addWidget(courseScheduleButton);

	QPushButton* tabelButton = new QPushButton("���Ի���", this);
	layout->addWidget(tabelButton);

	QPushButton* textButton = new QPushButton("�ı��Ի���", this);
	layout->addWidget(textButton);

	QPushButton* imageButton = new QPushButton("ͼƬ�Ի���", this);
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