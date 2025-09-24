#pragma execution_character_set("utf-8")
#include "TACPrepareClassDialog.h"
#include "common.h"
TACPrepareClassDialog::TACPrepareClassDialog(QWidget *parent)
	: TADialog(parent)
{
    this->setObjectName("TACPrepareClassDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(650, 700));
    this->setTitle("课前准备");
}

TACPrepareClassDialog::~TACPrepareClassDialog()
{}
