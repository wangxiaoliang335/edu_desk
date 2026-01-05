#pragma execution_character_set("utf-8")
#include "TACCountDownDialog.h"
#include <QLineEdit>
#include <QDate>
#include <QSettings>
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

    calendar = new TACalendarWidget(this);

    calendar->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    calendar->setBorderColor(WIDGET_BORDER_COLOR);
    calendar->setBorderWidth(WIDGET_BORDER_WIDTH);
    calendar->setRadius(40);
    // 初始化时先按本地配置填充（showEvent 里也会每次刷新一遍，保证取消后不残留）
    QSettings settings;
    const QDate savedDate = settings.value("countdown/date").toDate();
    m_date = savedDate.isValid() ? savedDate : QDate(2026, 6, 7);
    calendar->setSelectedDate(m_date);
    
    QLabel* label = new QLabel("高考时间", this);
    calendarButton = new QPushButton(this);
    calendarButton->setText(m_date.toString("yyyy年M月d日"));
    calendarButton->setObjectName("calendarButton");
    calendarButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    calendarButton->setLayoutDirection(Qt::RightToLeft);
    connect(calendarButton, &QPushButton::clicked, this, [=]() {
        calendar->setFixedWidth(calendarButton->width());
        QPoint bottomLeft = calendarButton->mapToGlobal(QPoint(0, calendarButton->height()));
        calendar->move(bottomLeft.x(), bottomLeft.y()+5);
        calendar->show();
    });
    connect(calendar, &TACalendarWidget::selectionChanged, this, [=]() {
        m_date = calendar->selectedDate();
        calendarButton->setText(m_date.toString("yyyy年M月d日"));
        calendar->close();
        countDownLabel->setText(QString::number(daysLeft()));
    });
    this->contentLayout->addWidget(label);
    this->contentLayout->addWidget(calendarButton);

    contentLineEdit = new QLineEdit(this);
    contentLineEdit->setText(settings.value("countdown/prefix", QString::fromUtf8(u8"距高考还有")).toString());
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

    // 关键：TADialog 里的“确定/取消”只会发出 enterClicked/cancelClicked 信号，
    // 这里需要显式连接，否则按钮点击没有任何效果。
    connect(this, &TADialog::cancelClicked, this, [=]() {
        // 取消：回滚到上次保存的状态（该对话框实例会被复用，下次打开要看到已保存的数据）
        QSettings s;
        const QDate d = s.value("countdown/date").toDate();
        m_date = d.isValid() ? d : QDate(2026, 6, 7);
        if (calendar)
            calendar->setSelectedDate(m_date);
        if (calendarButton)
            calendarButton->setText(m_date.toString("yyyy年M月d日"));
        if (contentLineEdit)
            contentLineEdit->setText(s.value("countdown/prefix", QString::fromUtf8(u8"距高考还有")).toString());
        if (countDownLabel)
            countDownLabel->setText(QString::number(daysLeft()));
        if (calendar)
            calendar->close();
        this->close();
    });
    connect(this, &TADialog::enterClicked, this, [=]() {
        // 保存配置并通知外部刷新
        QSettings s;
        s.setValue("countdown/date", m_date);
        s.setValue("countdown/prefix", contentLineEdit->text());
        emit done();
        if (calendar)
            calendar->close();
        this->close();
    });
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

void TACCountDownDialog::showEvent(QShowEvent * event)
{
    // 每次展示都从本地配置重新加载，避免“取消后仍保留未保存修改”的体验问题
    QSettings s;
    const QDate d = s.value("countdown/date").toDate();
    m_date = d.isValid() ? d : QDate(2026, 6, 7);
    if (calendar)
        calendar->setSelectedDate(m_date);
    if (calendarButton)
        calendarButton->setText(m_date.toString("yyyy年M月d日"));
    if (contentLineEdit)
        contentLineEdit->setText(s.value("countdown/prefix", QString::fromUtf8(u8"距高考还有")).toString());

    countDownLabel->setText(QString::number(daysLeft()));
    QWidget::showEvent(event);
}

void TACCountDownDialog::closeEvent(QCloseEvent* event)
{
    if (calendar)
        calendar->close();
    TADialog::closeEvent(event);
}
