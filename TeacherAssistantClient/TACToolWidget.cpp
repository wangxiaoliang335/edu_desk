#include "TACToolWidget.h"

#define ICON_SIZE QSize(25,20)
TACToolWidget::TACToolWidget(QWidget *parent)
	: QWidget(parent)
{
	this->setObjectName("TACToolWidget");
	QHBoxLayout* layout = new QHBoxLayout();
	layout->setSpacing(5);
	layout->setContentsMargins(5, 5, 5, 5);

	QPushButton* fontIncreaseButton = new QPushButton(this);
	fontIncreaseButton->setIcon(QIcon(":/res/img/font-increase.png"));
	fontIncreaseButton->setIconSize(ICON_SIZE);
	layout->addWidget(fontIncreaseButton);

	QPushButton* fontDecreaseButton = new QPushButton(this);
	fontDecreaseButton->setIcon(QIcon(":/res/img/font-decrease.png"));
	fontDecreaseButton->setIconSize(ICON_SIZE);
	layout->addWidget(fontDecreaseButton);

	QPushButton* fontUnderlineButton = new QPushButton(this);
	fontUnderlineButton->setIcon(QIcon(":/res/img/font-underline.png"));
	fontUnderlineButton->setIconSize(ICON_SIZE);
	layout->addWidget(fontUnderlineButton);

	QPushButton* fontFillButton = new QPushButton(this);
	fontFillButton->setIcon(QIcon(":/res/img/font-fill.png"));
	fontFillButton->setIconSize(ICON_SIZE);
	layout->addWidget(fontFillButton);
	layout->addStretch();

	this->setLayout(layout);

}

TACToolWidget::~TACToolWidget()
{}
