#pragma execution_character_set("utf-8")
#include "TADialog.h"
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QBrush>
#include <QApplication>
#include <QScreen>
TADialog::TADialog(QWidget *parent)
	: TABaseDialog(parent),
    buttonWidget(new QWidget(this)),
    cancelButton(new QPushButton("", this)),
    enterButton(new QPushButton("", this)),
    buttonLayout(new QHBoxLayout(buttonWidget))
{
    // 设置按钮文本（使用 UTF-8 编码避免乱码）
    cancelButton->setText(QString::fromUtf8(u8"取消"));
    enterButton->setText(QString::fromUtf8(u8"确定"));
    
    this->titleLabel->setAlignment(Qt::AlignCenter);
    buttonWidget->setObjectName("buttonWidget");
    buttonLayout->setContentsMargins(5, 5, 5, 5);
    buttonLayout->setSpacing(20);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(enterButton);
    buttonLayout->addStretch();
    mainLayout->addWidget(buttonWidget);
    enterButton->setObjectName("enterButton");
    connect(cancelButton, &QPushButton::clicked, this, &TADialog::onCancelClicked);
    connect(enterButton, &QPushButton::clicked, this, &TADialog::onEnterClicked);
}
void TADialog::onCancelClicked()
{
    emit cancelClicked();
}

void TADialog::onEnterClicked()
{
    emit enterClicked();
}

TADialog::~TADialog()
{}
