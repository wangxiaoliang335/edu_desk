#pragma execution_character_set("utf-8")
#include "TACFolderDialog.h"
#include <QScrollArea>
#include <QPushButton>
#include <QToolButton>
#include "common.h"
TACFolderDialog::TACFolderDialog(QWidget *parent)
	: TADialog(parent)
{
    this->setObjectName("TACFolderDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(650, 550));
    this->setTitle("文件管理");

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* container = new QWidget();
    QGridLayout* gridLayout = new QGridLayout(container);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(10, 10, 10, 10);

    scrollArea->setWidget(container);
    this->contentLayout->addWidget(scrollArea);

    QToolButton* folderButton = new QToolButton(this);
    folderButton->setIcon(QIcon(":/res/img/home_ic_file_nor.png"));
    folderButton->setIconSize(QSize(60, 60));
    folderButton->setText("文件夹1");
    folderButton->setFixedSize(100, 100);
    folderButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    gridLayout->addWidget(folderButton, 0, 0, Qt::AlignLeft | Qt::AlignTop);

    QToolButton* createFolderButton = new QToolButton(this);
    createFolderButton->setIcon(QIcon(":/res/img/text_popup_ic_add_nor.png"));
    createFolderButton->setIconSize(QSize(60, 60));
    createFolderButton->setText("新建文件夹");
    createFolderButton->setFixedSize(100, 100);
    createFolderButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    
    gridLayout->addWidget(createFolderButton, 0, 1, Qt::AlignLeft | Qt::AlignTop);
    
    //this->contentLayout->setContentsMargins(20, 10, 20, 10);
    
}

TACFolderDialog::~TACFolderDialog()
{}
