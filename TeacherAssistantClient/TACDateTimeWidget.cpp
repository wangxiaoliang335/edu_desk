#pragma execution_character_set("utf-8")
#include <QDateTime>
#include <QMouseEvent>
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
    // 窗口大小：只显示时间时用较小尺寸，显示日期时需要更宽
    this->setMinimumSize(QSize(200, 126));
    this->resize(QSize(200, 126));

   
    m_type = 0b01;
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        if (m_type == 0b11)
        {
            upContentLabel->show();
            QFont timeFont = upContentLabel->font();
            timeFont.setPointSize(36); // 时间+日期模式，时间使用中等字体
            upContentLabel->setFont(timeFont);
            upContentLabel->setText(QTime::currentTime().toString("hh:mm"));
            downContentLabel->show();
            QFont dateFont = downContentLabel->font();
            dateFont.setPointSize(20); // 日期使用较小字体
            downContentLabel->setFont(dateFont);
            downContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
        }
        else if (m_type == 0b10)
        {
            upContentLabel->show();
            // 只显示日期时，使用较大的字体大小
            QFont dateFont = upContentLabel->font();
            dateFont.setPointSize(32); // 日期使用较大的字体
            upContentLabel->setFont(dateFont);
            upContentLabel->setText(QDate::currentDate().toString("MM月dd日"));
            downContentLabel->hide();
        }
        else
        {
            upContentLabel->show();
            // 只显示时间时，使用中等字体大小
            QFont timeFont = upContentLabel->font();
            timeFont.setPointSize(36); // 时间使用中等字体，避免超出窗口
            upContentLabel->setFont(timeFont);
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
    // 根据显示类型调整窗口大小
    if (m_type == 0b10) {
        // 只显示日期，需要更宽的窗口
        this->resize(QSize(240, 126));
    } else if (m_type == 0b11) {
        // 显示时间和日期，需要更宽的窗口
        this->resize(QSize(240, 126));
    } else {
        // 只显示时间，使用较小窗口
        this->resize(QSize(200, 126));
    }
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

void TACDateTimeWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		emit doubleClicked();
	}
	TAFloatingWidget::mouseDoubleClickEvent(event);
}
