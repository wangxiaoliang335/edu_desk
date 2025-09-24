#pragma execution_character_set("utf-8")
#include "TACCountDownDialog.h"
#include <QLineEdit>
#include <QDate>
#include "common.h"
#include "TAConfig.h"
#include "util.h"
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


    calendar = new TACalendarWidget(this);

    calendar->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    calendar->setBorderColor(WIDGET_BORDER_COLOR);
    calendar->setBorderWidth(WIDGET_BORDER_WIDTH);
    calendar->setRadius(40);
    
    TACountDownInfo info = TAConfig::instance()->countDownInfo();
    m_date = info.date;
    QLabel* label = new QLabel("高考时间", this);
    QPushButton* calendarButton = new QPushButton(this);
    calendarButton->setText(info.date.toString("yyyy年M月d日"));
    calendarButton->setObjectName("calendarButton");
    calendarButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    calendarButton->setLayoutDirection(Qt::RightToLeft);
    connect(calendarButton, &QPushButton::clicked, this, [=]() {
        calendar->setFixedWidth(calendarButton->width());
        QPoint bottomLeft = calendarButton->mapToGlobal(QPoint(0, calendarButton->height()));
        calendar->move(bottomLeft.x(), bottomLeft.y()+5);
        calendar->show();
    });
    connect(calendar, &TACalendarWidget::selectionChanged, this, [=](QDate date) {

        this->m_date = date;
        calendarButton->setText(date.toString("yyyy年M月d日"));
        calendar->close();
        countDownLabel->setText(QString::number(daysLeft()));

        });
    this->contentLayout->addWidget(label);
    this->contentLayout->addWidget(calendarButton);

    contentLineEdit = new QLineEdit(this);
    contentLineEdit->setText(info.prefix);
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

    connect(this, &TACCountDownDialog::enterClicked, this, [=]() {
        TAConfig::instance()->countDownInfo().date = m_date;
        TAConfig::instance()->countDownInfo().prefix = contentLineEdit->text();
        TAConfig::instance()->countDownInfo().suffix = dayLabel->text();
        TAConfig::instance()->save();
        emit done();
        this->close();
    });
    connect(this, &TACCountDownDialog::cancelClicked, this, [=]() {this->close(); });
}

TACCountDownDialog::~TACCountDownDialog()
{}

int TACCountDownDialog::daysLeft()
{
    QDate today = QDate::currentDate();
    int days = today.daysTo(m_date);
    return days;
}

QString TACCountDownDialog::content()
{
    return contentLineEdit->text() + QString::number(daysLeft()) + "天";
}

QDate TACCountDownDialog::date()
{
    return m_date;
}

void TACCountDownDialog::showEvent(QShowEvent * event)
{
    countDownLabel->setText(QString::number(daysLeft()));
    QWidget::showEvent(event);
}

void TACCountDownDialog::closeEvent(QCloseEvent* event)
{
    TADialog::closeEvent(event);
    this->calendar->close();
}
