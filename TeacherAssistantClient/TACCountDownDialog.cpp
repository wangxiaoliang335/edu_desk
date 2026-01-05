#pragma execution_character_set("utf-8")
#include "TACCountDownDialog.h"
#include <QLineEdit>
#include <QDate>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QSignalBlocker>
#include "common.h"

namespace {
static QString countdownSettingsPath()
{
    // 固定到可写配置目录，避免 QSettings 因组织名/应用名/权限差异导致读写不一致
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!dir.isEmpty())
        QDir().mkpath(dir);
    // 兜底：如果拿不到可写目录，则写到当前工作目录
    return dir.isEmpty() ? QStringLiteral("./tac_countdown.ini")
                         : (dir + QStringLiteral("/tac_countdown.ini"));
}
} // namespace
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
    QSettings settings(countdownSettingsPath(), QSettings::IniFormat);
    settings.sync(); // 确保从磁盘读取最新值
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
        // 打开时让弹窗定位到当前选择日期所在月份（避免第一次打开总是落在 1 月）
        if (calendar) {
            const QSignalBlocker blocker(calendar); // 防止触发 selectionChanged 导致弹窗立刻关闭
            calendar->setSelectedDate(m_date);
        }
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
    QString savedPrefix = settings.value("countdown/prefix", QString::fromUtf8(u8"距高考还有")).toString();
    contentLineEdit->setText(savedPrefix);
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
        QSettings s(countdownSettingsPath(), QSettings::IniFormat);
        s.sync(); // 确保从磁盘读取最新值
        const QDate d = s.value("countdown/date").toDate();
        m_date = d.isValid() ? d : QDate(2026, 6, 7);
        if (calendar)
            calendar->setSelectedDate(m_date);
        if (calendarButton)
            calendarButton->setText(m_date.toString("yyyy年M月d日"));
        if (contentLineEdit) {
            QString savedPrefix = s.value("countdown/prefix", QString::fromUtf8(u8"距高考还有")).toString();
            contentLineEdit->setText(savedPrefix);
        }
        if (countDownLabel)
            countDownLabel->setText(QString::number(daysLeft()));
        if (calendar)
            calendar->close();
        this->close();
    });
    connect(this, &TADialog::enterClicked, this, [=]() {
        // 保存配置并通知外部刷新
        QSettings s(countdownSettingsPath(), QSettings::IniFormat);
        QString prefixText = contentLineEdit->text().trimmed(); // 获取并去除首尾空格
        if (prefixText.isEmpty()) {
            prefixText = QString::fromUtf8(u8"距高考还有"); // 如果为空，使用默认值
        }
        s.setValue("countdown/date", m_date);
        s.setValue("countdown/prefix", prefixText);
        s.sync(); // 确保立即写入磁盘
        
        qDebug() << "TACCountDownDialog::enterClicked - 保存提示文字:" << prefixText
                 << ", settings file:" << s.fileName();
        
        // 确保 contentLineEdit 显示的是保存后的值
        contentLineEdit->setText(prefixText);
        
        emit done(); // 发出信号，通知主窗口刷新显示
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
    // 每次展示都从本地配置重新加载，避免"取消后仍保留未保存修改"的体验问题
    QSettings s(countdownSettingsPath(), QSettings::IniFormat);
    s.sync(); // 确保从磁盘读取最新值
    
    // 读取日期
    const QDate d = s.value("countdown/date").toDate();
    m_date = d.isValid() ? d : QDate(2026, 6, 7);
    if (calendar)
        calendar->setSelectedDate(m_date);
    if (calendarButton)
        calendarButton->setText(m_date.toString("yyyy年M月d日"));
    
    // 读取提示文字 - 确保读取到最新保存的值
    if (contentLineEdit) {
        QString savedPrefix = s.value("countdown/prefix").toString();
        // 如果读取的值为空，才使用默认值
        if (savedPrefix.isEmpty()) {
            savedPrefix = QString::fromUtf8(u8"距高考还有");
        }
        contentLineEdit->setText(savedPrefix);
        qDebug() << "TACCountDownDialog::showEvent - 读取提示文字:" << savedPrefix
                 << ", settings file:" << s.fileName();
    }

    countDownLabel->setText(QString::number(daysLeft()));
    QWidget::showEvent(event);
}

void TACCountDownDialog::closeEvent(QCloseEvent* event)
{
    if (calendar)
        calendar->close();
    TADialog::closeEvent(event);
}
