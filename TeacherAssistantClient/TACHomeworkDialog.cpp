#pragma execution_character_set("utf-8")
#include "TACHomeworkDialog.h"
#include "common.h"
TACHomeworkDialog::TACHomeworkDialog(QWidget *parent)
	: TABaseDialog(parent)
{
    this->setObjectName("TACHomeworkDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(650, 450));
    this->setTitle("家庭作业");
    this->titleLabel->setAlignment(Qt::AlignCenter);

    contentLabel = new QLabel(this);
    contentLabel->setObjectName("contentLabel");
    contentLabel->setWordWrap(true);
    contentLabel->setAlignment(Qt::AlignTop);
    contentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->contentLayout->addWidget(contentLabel);
}

TACHomeworkDialog::~TACHomeworkDialog()
{}

void TACHomeworkDialog::setContent(const QString & text)
{
    this->contentLabel->setText(text);
}
