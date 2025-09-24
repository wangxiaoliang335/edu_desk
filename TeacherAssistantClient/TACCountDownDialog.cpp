#pragma execution_character_set("utf-8")
#include "TACCountDownDialog.h"
#include <QLineEdit>
#include <QDate>
#include "common.h"
TACCountDownDialog::TACCountDownDialog(QWidget *parent)
	: TADialog(parent)
{
    this->setObjectName("TACCountDownDialog");
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(40);
    this->setFixedSize(QSize(650, 630));
    this->setTitle("倒计时");
    this->contentLayout->setSpacing(20);

    TACalendarWidget* calendar = new TACalendarWidget();

    calendar->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    calendar->setBorderColor(WIDGET_BORDER_COLOR);
    calendar->setBorderWidth(WIDGET_BORDER_WIDTH);
    calendar->setRadius(40);
    
    QLabel* label = new QLabel("高考时间", this);
    QPushButton* calendarButton = new QPushButton(this);
    calendarButton->setText("2026-06-07");
    calendarButton->setObjectName("calendarButton");
    calendarButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    calendarButton->setLayoutDirection(Qt::RightToLeft);
    connect(calendarButton, &QPushButton::clicked, this, [=]() {
        //calendarDialog->setFixedWidth(calendarButton->width());
        QPoint bottomLeft = calendarButton->mapToGlobal(QPoint(0, calendarButton->height()));
        calendar->move(bottomLeft.x(), bottomLeft.y()+5);
        calendar->show();
    });
    this->contentLayout->addWidget(label);
    this->contentLayout->addWidget(calendarButton);

    contentLineEdit = new QLineEdit(this);
    contentLineEdit->setText("距高考还有");
    QLabel* label1 = new QLabel("提示文字", this);
    this->contentLayout->addWidget(label1);
    this->contentLayout->addWidget(contentLineEdit);

    QWidget* spacer = new QWidget(this);
    spacer->setFixedHeight(20);
    this->contentLayout->addWidget(spacer);
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addStretch();
    countDownLabel = new QLabel(this);
    countDownLabel->setFixedSize(QSize(136, 116));
    countDownLabel->setObjectName("countDownLabel");
    countDownLabel->setAlignment(Qt::AlignCenter);
    //countDownLabel->setPixmap(QPixmap(":/res/img/countdown_popup_bg.png"));

    QLabel* dayLabel = new QLabel(this);
    dayLabel->setText("天");

    hLayout->addWidget(countDownLabel);
    hLayout->addWidget(dayLabel);
    hLayout->addStretch();
    this->contentLayout->addLayout(hLayout);
    this->contentLayout->addStretch();
}

TACCountDownDialog::~TACCountDownDialog()
{}

int TACCountDownDialog::daysLeft()
{
    QDate today = QDate::currentDate();
    QDate gaokaoDate(2026, 6, 7);
    int days = today.daysTo(gaokaoDate);
    return days;
}

QString TACCountDownDialog::content()
{
    return contentLineEdit->text() + QString::number(daysLeft()) + "天";
}

void TACCountDownDialog::showEvent(QShowEvent * event)
{
   
    countDownLabel->setText(QString::number(daysLeft()));
    QWidget::showEvent(event);
}
