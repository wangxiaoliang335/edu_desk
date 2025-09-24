#pragma execution_character_set("utf-8")
#include "TACLogoWidget.h"
#include "common.h"
#include <QPixmap>
#include <QHBoxLayout>
TACLogoWidget::TACLogoWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
    this->setObjectName("TACLogoWidget");
    
    this->resize(QSize(140, 70));
}

TACLogoWidget::~TACLogoWidget()
{}
void TACLogoWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (!m_fileName.isEmpty())
    {
        QPixmap m_backgroundPixmap = QPixmap(m_fileName);
        QPixmap scaled = m_backgroundPixmap.scaled(
            size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        QRect r = scaled.rect();
        r.moveCenter(rect().center());
        painter.drawPixmap(r, scaled);
    }
}
void TACLogoWidget::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    int x = 50;
    int y = 55;
    this->move(x, y);
}
void TACLogoWidget::updateLogo(const QString & fileName)
{
    m_fileName = fileName;
    update();
}
void TACLogoWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}

TACSchoolLabelWidget::TACSchoolLabelWidget(QWidget* parent) : TAFloatingWidget(parent)
{
    this->setObjectName("TACSchoolLabelWidget");
    label = new QLabel("学校名称",this);
    label->setAlignment(Qt::AlignCenter);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    this->setLayout(layout);

    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);
    this->resize(140,70);

}

TACSchoolLabelWidget::~TACSchoolLabelWidget()
{
}

void TACSchoolLabelWidget::setContent(const QString& text)
{
    label->setText(text);
}

void TACSchoolLabelWidget::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    int x = 200;
    int y = 55;
    this->move(x, y);
}
void TACSchoolLabelWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}
TACClassLabelWidget::TACClassLabelWidget(QWidget* parent) : TAFloatingWidget(parent)
{
    this->setObjectName("TACClassLabelWidget");
    label = new QLabel("班级名称",this);
    label->setAlignment(Qt::AlignCenter);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    this->setLayout(layout);

    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);
    this->resize(140, 70);
}

TACClassLabelWidget::~TACClassLabelWidget()
{
}

void TACClassLabelWidget::setContent(const QString& text)
{
    label->setText(text);
}
void TACClassLabelWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked();
    }
}
void TACClassLabelWidget::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    int x = 360;
    int y = 55;
    this->move(x, y);
}