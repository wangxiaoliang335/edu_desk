#pragma execution_character_set("utf-8")
#include "TACLogoDialog.h"
#include <QToolButton>
#include "common.h"
TACLogoDialog::TACLogoDialog(QWidget *parent)
	: TADialog(parent)
{
	this->setObjectName("TACLogoDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(650, 670));
    this->setTitle("文本/图标");

    this->contentLayout->setContentsMargins(20, 10, 20, 10);
    this->contentLayout->setSpacing(20);
    QLabel* schoolNameLabel = new QLabel("学校名称", this);
    schoolNameEdit = new QLineEdit(this);
    schoolNameEdit->setPlaceholderText("请输入学校名称");
    this->contentLayout->addWidget(schoolNameLabel);
    this->contentLayout->addWidget(schoolNameEdit);

    QWidget* spacer0 = new QWidget(this);
    spacer0->setFixedSize(10, 10);
    this->contentLayout->addWidget(spacer0);

    QLabel* classNameLabel = new QLabel("班级名称", this);
    classNameEdit = new QLineEdit(this);
    classNameEdit->setPlaceholderText("请输入班级名称");
    this->contentLayout->addWidget(classNameLabel);
    this->contentLayout->addWidget(classNameEdit);

    QWidget* spacer1 = new QWidget(this);
    spacer1->setFixedSize(10, 10);
    this->contentLayout->addWidget(spacer1);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(20);

    QToolButton* logoButton = new QToolButton(this);
    logoButton->setObjectName("logoButton");
    logoButton->setIcon(QIcon(":/res/img/text_popup_ic_add_nor.png"));
    logoButton->setIconSize(QSize(40, 40));
    logoButton->setText("添加学校LOGO");
    logoButton->setFixedSize(140,140);
    logoButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QToolButton* otherButton = new QToolButton(this);
    otherButton->setObjectName("otherButton");
    otherButton->setIcon(QIcon(":/res/img/text_popup_ic_add_nor.png"));
    otherButton->setIconSize(QSize(40, 40));
    otherButton->setText("添加荣誉图标");
    otherButton->setFixedSize(140, 140);
    otherButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    buttonsLayout->addWidget(logoButton);
    buttonsLayout->addWidget(otherButton);
    buttonsLayout->setAlignment(Qt::AlignLeft);

    this->contentLayout->addLayout(buttonsLayout);
    this->contentLayout->addStretch();


}

TACLogoDialog::~TACLogoDialog()
{}

QString TACLogoDialog::getSchoolName() const
{
    return schoolNameEdit->text();
}

QString TACLogoDialog::getClassName()
{
    return classNameEdit->text();
}

QString TACLogoDialog::getLogoFileName()
{
    return logoFileName;
}

QString TACLogoDialog::getOtherFileName()
{
    return otherFileName;
}
