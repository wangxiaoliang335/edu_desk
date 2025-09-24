#pragma execution_character_set("utf-8")
#include "TACWallpaperLibraryDialog.h"
#include <QScrollArea>
#include <QPainterPath>
#include "common.h"
TACWallpaperLibraryDialog::TACWallpaperLibraryDialog(QWidget *parent)
	: TABaseDialog(parent)
{
    this->setObjectName("TACWallpaperLibraryDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(890, 550));
    this->setTitle("壁纸库");

    QPushButton* downloadAsWallpaperButton = new QPushButton("下载为壁纸", this);
    QPushButton* downloadButton = new QPushButton("下载", this);
    this->titleLayout->addStretch();
    this->titleLayout->addWidget(downloadAsWallpaperButton);
    this->titleLayout->addWidget(downloadButton);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* container = new QWidget();
    gridLayout = new QGridLayout(container);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(10, 10, 10, 10);

    scrollArea->setWidget(container);
    this->contentLayout->addWidget(scrollArea);
    init();
}

TACWallpaperLibraryDialog::~TACWallpaperLibraryDialog()
{}

void TACWallpaperLibraryDialog::init()
{
    for (int i = 1; i < 5; ++i)
    {
        TACWallpaperItemWidget* item = new TACWallpaperItemWidget(this);
        item->setBackgroundImage("C:/workspace/obs/TeacherAssistant/TeacherAssistantClient/res/bg/"+QString::number(i) + ".jpg");
        gridLayout->addWidget(item, 0, i - 1,Qt::AlignLeft|Qt::AlignTop);
    }
    TACWallpaperItemWidget* item = new TACWallpaperItemWidget(this);
    item->setBackgroundImage("C:/workspace/obs/TeacherAssistant/TeacherAssistantClient/res/bg/5.jpg");
    gridLayout->addWidget(item, 1, 0, Qt::AlignLeft | Qt::AlignTop);
}

TACWallpaperItemWidget::TACWallpaperItemWidget(QWidget* parent) : QWidget(parent), hover(false) {
    setMinimumSize(200, 123);
    setMaximumSize(200, 123);
    //backgroundImage.load(":/default/path/to/image.png");
}

void TACWallpaperItemWidget::setBackgroundImage(const QString& path)
{
    if (backgroundImage.load(path)) {
        update();
    }
    
}
void TACWallpaperItemWidget::paintEvent(QPaintEvent*){
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    QRect rectArea = rect().adjusted(0, 0, -1, -1);
    path.addRoundedRect(rectArea, 15, 15);
    painter.setClipPath(path);
    painter.drawPixmap(rect(), backgroundImage);

    if (hover) {
        painter.setPen(QPen(QColor(255,255,255), 1));
    }
    else {
        painter.setPen(QPen(Qt::NoPen));
    }
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(0,0,-1,-1), 15, 15);
}
void TACWallpaperItemWidget::enterEvent(QEvent* event)
{
    hover = true;
    update();
    QWidget::enterEvent(event);
}

void TACWallpaperItemWidget::leaveEvent(QEvent* event)
{
    hover = false;
    update();
    QWidget::leaveEvent(event);
}