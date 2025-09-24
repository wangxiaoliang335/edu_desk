#pragma execution_character_set("utf-8")
#include <QDateTime>
#include "TACDateTimeWidget.h"
#include "common.h"
TACDateTimeWidget::TACDateTimeWidget(QWidget *parent)
	: TAFloatingWidget(parent)
{
    this->setObjectName("TACDateTimeWidget");
    upContentLabel = new QLabel(this);
    upContentLabel->setAlignment(Qt::AlignCenter);
    upContentLabel->setObjectName("upContentLabel");

    
    
    downContentLabel = new QLabel(this);
    downContentLabel->setAlignment(Qt::AlignCenter);
    downContentLabel->setObjectName("downContentLabel");
    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setAlignment(Qt::AlignCenter);
    vLayout->addWidget(upContentLabel);
    vLayout->addWidget(downContentLabel);
    this->setLayout(vLayout);

    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);
    this->setFixedSize(QSize(200, 126));

   
    m_type = 0b01;
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        if (m_type == 0b11)
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            downContentLabel->show();
            downContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
        }
        else if (m_type == 0b10)
        {
            upContentLabel->show();
            upContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
            downContentLabel->hide();
        }
        else
        {
            upContentLabel->show();
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            downContentLabel->hide();
        }
    });
    timer->start(1000);
}

TACDateTimeWidget::~TACDateTimeWidget()
{}
void TACDateTimeWidget::setType(int type)
{
    m_type = type;
}
void TACDateTimeWidget::initShow()
{
	QRect rect = this->getScreenGeometryWithTaskbar();
	if (rect.isEmpty())
		return;
	QSize windowSize = this->size();
	int x = rect.x() + rect.width() - 30 - this->width();
	int y = rect.y() + 60;
	this->move(x, y);
}

