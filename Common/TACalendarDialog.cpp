#pragma execution_character_set("utf-8")
#include "TACalendarDialog.h"
#include "CommonInfo.h"
#include <QPainterPath>
#include <QVariant>
#include <QResizeEvent>
#include <QAbstractButton>
#include <QHeaderView>
#include <QFont>
#include <QFontMetrics>
#include <QScrollBar>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPushButton>
#include <QHBoxLayout>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QUrl>
#include <QUrlQuery>
// ==================== TACalendarToastWidget（自定义提示窗口） ====================

TACalendarToastWidget::TACalendarToastWidget(QWidget* parent)
    : QWidget(parent)
    , m_type(Info)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    
    // 设置固定大小
    setFixedSize(300, 80);
    
    // 初始化定时器和动画
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &TACalendarToastWidget::startFadeOut);
    
    m_fadeAnimation = new QPropertyAnimation(this, "opacity", this);
    m_fadeAnimation->setDuration(300);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::close);
}

void TACalendarToastWidget::setOpacity(qreal opacity)
{
    m_opacity = opacity;
    update();
}

void TACalendarToastWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(m_opacity);
    
    // 背景颜色（比校历窗口深一些：校历窗口是43,43,43，这里用25,25,25）
    QColor bgColor(25, 25, 25);
    QColor borderColor(120, 120, 120);
    
    QRect rect(0, 0, width(), height());
    QPainterPath path;
    path.addRoundedRect(rect.adjusted(0, 0, -1, -1), 8, 8);
    
    painter.fillPath(path, QBrush(bgColor));
    
    QPen pen(borderColor);
    pen.setWidth(1);
    painter.strokePath(path, pen);
    
    // 绘制文字
    painter.setPen(QColor(255, 255, 255, 230));
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);
    
    QRect textRect = rect.adjusted(15, 10, -15, -10);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, m_message);
}

void TACalendarToastWidget::positionAtBottomRight()
{
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    
    QRect screenGeometry = screen->geometry();
    int x = screenGeometry.right() - width() - 30;
    int y = screenGeometry.bottom() - height() - 30;
    
    move(x, y);
}

void TACalendarToastWidget::startFadeOut()
{
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.0);
    m_fadeAnimation->start();
}

void TACalendarToastWidget::showToast(QWidget* parent, const QString& message, ToastType type, int duration)
{
    TACalendarToastWidget* toast = new TACalendarToastWidget(parent);
    toast->m_message = message;
    toast->m_type = type;
    
    // 根据消息长度动态调整窗口大小
    QFont font;
    font.setPointSize(12);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(message);
    int maxWidth = 400;
    int minWidth = 300;
    int width = qBound(minWidth, textWidth + 40, maxWidth);
    
    // 计算文本高度
    QRect textRect = fm.boundingRect(QRect(0, 0, width - 30, 0), Qt::TextWordWrap, message);
    int height = qMax(60, textRect.height() + 30);
    
    toast->setFixedSize(width, height);
    toast->positionAtBottomRight();
    toast->show();
    
    // 启动定时器，duration毫秒后开始淡出
    toast->m_timer->start(duration);
}

TACalendarWidget::TACalendarWidget(QWidget* parent)
    : QWidget(parent)
    , m_currentDate(QDate::currentDate())
    , m_selectedDate(QDate::currentDate())
{
    setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnTopHint
    );
    setAttribute(Qt::WA_TranslucentBackground);
    this->setObjectName("TACalendarWidget");
    setMouseTracking(true);

    // 关闭按钮（默认隐藏，鼠标移入显示）
    m_closeButton = new QToolButton(this);
    m_closeButton->setText(QStringLiteral("×"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
        "QToolButton {"
        "  border: none;"
        "  color: rgba(255,255,255,0.9);"
        "  background: rgba(0,0,0,0.25);"
        "  border-radius: 12px;"
        "  font-weight: bold;"
        "}"
        "QToolButton:hover { background: rgba(255,0,0,0.35); }"
    );
    m_closeButton->hide();
    m_closeButton->raise();
    connect(m_closeButton, &QToolButton::clicked, this, [this]() {
        this->hide(); // 不销毁，便于复用指针
    });
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(20,20,20,20);

    QHBoxLayout* navLayout = new QHBoxLayout();
    navLayout->setSpacing(10);
    navLayout->addStretch();
    m_yearButton = new QPushButton(this);
    m_yearButton->setObjectName("yearButton");
    connect(m_yearButton, &QPushButton::clicked, this, [=]() {
        QPoint pos = m_yearButton->mapToGlobal(QPoint(0, m_yearButton->height()));
        m_yearMenu->exec(pos);
    });
    m_yearMenu = new QMenu(this);

    m_monthButton = new QPushButton(this);
    m_monthButton->setObjectName("monthButton");
    connect(m_monthButton, &QPushButton::clicked, this, [=]() {
        QPoint pos = m_monthButton->mapToGlobal(QPoint(0, m_monthButton->height()));
        m_monthMenu->exec(pos);
        });
    m_monthMenu = new QMenu(this);

    m_yearButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    m_yearButton->setLayoutDirection(Qt::RightToLeft);
    m_monthButton->setIcon(QIcon(":/res/img/arrow-down.png"));
    m_monthButton->setLayoutDirection(Qt::RightToLeft);
    navLayout->addWidget(m_yearButton);
    navLayout->addWidget(m_monthButton);
    navLayout->addStretch();

    m_calendarLayout = new QGridLayout();
    m_calendarLayout->setSpacing(2);
    m_calendarLayout->setContentsMargins(0, 0, 0, 0);

    m_mainLayout->addLayout(navLayout);
    m_mainLayout->addLayout(m_calendarLayout);

    createWeekdayLabels();
    populateYearCombo();
    populateMonthCombo();
    updateCalendar();

    onYearChanged(m_currentDate.year());
    onMonthChanged(m_currentDate.month());
}

void TACalendarWidget::enterEvent(QEvent* event)
{
    QWidget::enterEvent(event);
    if (m_closeButton) m_closeButton->show();
}

void TACalendarWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    if (m_closeButton) m_closeButton->hide();
}

void TACalendarWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_closeButton) {
        const int margin = 10;
        m_closeButton->move(width() - m_closeButton->width() - margin, margin);
    }
}

void TACalendarWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 避免与按钮点击冲突：点到按钮就交给按钮处理
        QWidget* child = childAt(event->pos());
        if (!qobject_cast<QAbstractButton*>(child)) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TACalendarWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TACalendarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

// ==================== TASchoolCalendarWidget（学期校历表） ====================

static QSet<QDate> buildRangeDates(const QDate& start, const QDate& endInclusive)
{
    QSet<QDate> s;
    if (!start.isValid() || !endInclusive.isValid()) return s;
    QDate d = start;
    while (d.isValid() && d <= endInclusive) {
        s.insert(d);
        d = d.addDays(1);
    }
    return s;
}

TASchoolCalendarWidget::TASchoolCalendarWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setObjectName("TASchoolCalendarWidget");
    
    // 默认：深色底（与桌面管理一致时文字需白色）
    m_backgroundColor = QColor(43, 43, 43);
    m_borderColor = QColor(120, 120, 120);
    m_borderWidth = 1;
    m_radius = 10;

    // 关闭按钮（默认隐藏，鼠标移入显示）
    m_closeButton = new QToolButton(this);
    m_closeButton->setText(QStringLiteral("×"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
        "QToolButton { border:none; color:#ffffff; background: rgba(255,255,255,0.12); border-radius:12px; font-weight:bold; }"
        "QToolButton:hover { background: rgba(255,0,0,0.35); }"
    );
    m_closeButton->hide();
    m_closeButton->raise();
    connect(m_closeButton, &QToolButton::clicked, this, [this]() { hide(); });

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 14);
    mainLayout->setSpacing(8);

    m_schoolLabel = new QLabel(this);
    m_schoolLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    QFont f1 = m_schoolLabel->font();
    f1.setPointSize(18);
    f1.setBold(true);
    m_schoolLabel->setFont(f1);
    m_schoolLabel->setStyleSheet("color: rgba(255,255,255,0.92);");

    m_termLabel = new QLabel(this);
    m_termLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    QFont f2 = m_termLabel->font();
    f2.setPointSize(14);
    f2.setBold(true);
    m_termLabel->setFont(f2);
    m_termLabel->setStyleSheet("color: rgba(255,255,255,0.92);");

    mainLayout->addWidget(m_schoolLabel);
    mainLayout->addWidget(m_termLabel);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_table->setShowGrid(true);
    m_table->setWordWrap(true);
    m_table->setStyleSheet(
        "QTableWidget { background: transparent; gridline-color: rgba(255,255,255,0.25); color: rgba(255,255,255,0.92); }"
        // 表头背景用灰色（与窗口背景一致），避免白字看不清
        "QHeaderView::section { background-color: #2b2b2b; color: rgba(255,255,255,0.92); font-weight:bold; border:1px solid rgba(255,255,255,0.25); }"
    );
    mainLayout->addWidget(m_table, 1);

    // 导入按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* importBtn = new QPushButton(QString::fromUtf8(u8"导入校历"), this);
    importBtn->setStyleSheet(
        "QPushButton { "
        "background-color: #4169E1; "
        "color: white; "
        "border: none; "
        "padding: 8px 16px; "
        "border-radius: 4px; "
        "font-size: 14px; "
        "}"
        "QPushButton:hover { background-color: #5B7FD8; } "
        "QPushButton:pressed { background-color: #3357C7; }"
    );
    connect(importBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, 
            QString::fromUtf8(u8"选择校历文件"), 
            "", 
            QString::fromUtf8(u8"所有支持格式 (*.json *.xlsx *.xls);;JSON文件 (*.json);;Excel文件 (*.xlsx *.xls)"));
        if (!fileName.isEmpty()) {
            if (fileName.endsWith(".json", Qt::CaseInsensitive)) {
                importFromJson(fileName);
            } else if (fileName.endsWith(".xlsx", Qt::CaseInsensitive) || fileName.endsWith(".xls", Qt::CaseInsensitive)) {
                importFromExcel(fileName);
            }
        }
    });
    buttonLayout->addWidget(importBtn);
    mainLayout->addLayout(buttonLayout);
    
    m_legendLabel = new QLabel(this);
    m_legendLabel->setText(QString::fromUtf8(u8"注：蓝色表示工作日，绿色表示节假日。"));
    m_legendLabel->setStyleSheet("color: rgba(255,255,255,0.85); font-size:12px;");
    mainLayout->addWidget(m_legendLabel);

    // 默认窗口大小
    resize(1100, 820);
    
    // 初始化HTTP处理器（用于上传）
    m_httpHandler = new TAHttpHandler(this);
    connect(m_httpHandler, &TAHttpHandler::success, this, [this](const QString& response) {
        // 解析服务器响应
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8());
        if (jsonDoc.isObject()) {
            QJsonObject obj = jsonDoc.object();
            int code = obj["code"].toInt(0);
            QString message = obj["message"].toString();
            
            if (code == 200) {
                // 成功
                QString successMsg = QString::fromUtf8(u8"校历已成功上传到服务器！");
                if (!message.isEmpty()) {
                    successMsg += QString(" %1").arg(message);
                }
                TACalendarToastWidget::showToast(this, successMsg, TACalendarToastWidget::Success);
            } else {
                // 服务器返回错误
                QString errorMsg = message.isEmpty() ? QString::fromUtf8(u8"服务器返回错误") : message;
                TACalendarToastWidget::showToast(this, 
                    QString::fromUtf8(u8"校历上传失败：%1 (错误码: %2)").arg(errorMsg).arg(code),
                    TACalendarToastWidget::Error);
            }
        } else {
            // 响应格式不正确，但HTTP请求成功
            TACalendarToastWidget::showToast(this, 
                QString::fromUtf8(u8"校历已上传到服务器（响应格式异常）"),
                TACalendarToastWidget::Info);
        }
    });
    connect(m_httpHandler, &TAHttpHandler::failed, this, [this](const QString& error) {
        // 尝试解析错误响应
        QJsonDocument jsonDoc = QJsonDocument::fromJson(error.toUtf8());
        QString errorMsg;
        if (jsonDoc.isObject()) {
            QJsonObject obj = jsonDoc.object();
            errorMsg = obj["message"].toString();
            if (errorMsg.isEmpty()) {
                errorMsg = error;
            }
        } else {
            errorMsg = error.isEmpty() ? QString::fromUtf8(u8"网络请求失败") : error;
        }
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"校历上传到服务器失败：%1").arg(errorMsg),
            TACalendarToastWidget::Error);
    });
    
    // 初始化HTTP处理器（用于获取校历）
    m_httpHandlerForGet = new TAHttpHandler(this);
    connect(m_httpHandlerForGet, &TAHttpHandler::success, this, [this](const QString& response) {
        parseCalendarResponse(response);
    });
    connect(m_httpHandlerForGet, &TAHttpHandler::failed, this, [this](const QString& error) {
        // 获取失败时，不显示错误提示（可能是没有校历数据）
        qWarning() << "获取校历失败:" << error;
    });
    
    // 初始化时自动从服务器加载校历
    loadCalendarFromServer();
}

void TASchoolCalendarWidget::setBackgroundColor(const QColor& color) { m_backgroundColor = color; update(); }
void TASchoolCalendarWidget::setBorderColor(const QColor& color) { m_borderColor = color; update(); }
void TASchoolCalendarWidget::setBorderWidth(int val) { m_borderWidth = val; update(); }
void TASchoolCalendarWidget::setRadius(int val) { m_radius = val; update(); }

void TASchoolCalendarWidget::setCalendarInfo(const QString& schoolName,
                                             const QString& termTitle,
                                             const QDate& termStartMonday,
                                             int weekCount,
                                             const QString& remarkText,
                                             const QSet<QDate>& holidayDates,
                                             const QSet<QDate>& makeupWorkDates)
{
    m_schoolName = schoolName;
    m_termTitle = termTitle;
    m_termStartMonday = termStartMonday;
    m_weekCount = weekCount;
    m_remarkText = remarkText;
    m_holidayDates = holidayDates;
    m_makeupWorkDates = makeupWorkDates;

    if (m_schoolLabel) m_schoolLabel->setText(m_schoolName);
    if (m_termLabel) m_termLabel->setText(m_termTitle);

    rebuildTable();
}

void TASchoolCalendarWidget::setRemarksByDate(const QMap<QDate, QStringList>& remarksByDate)
{
    m_remarksByDate = remarksByDate;
    rebuildTable();
}

void TASchoolCalendarWidget::rebuildTable()
{
    if (!m_table || !m_termStartMonday.isValid() || m_weekCount <= 0) return;

    const QStringList headers = {
        QString::fromUtf8(u8"周次"),
        QString::fromUtf8(u8"一"),
        QString::fromUtf8(u8"二"),
        QString::fromUtf8(u8"三"),
        QString::fromUtf8(u8"四"),
        QString::fromUtf8(u8"五"),
        QString::fromUtf8(u8"六"),
        QString::fromUtf8(u8"日"),
        QString::fromUtf8(u8"备注")
    };

    m_table->clear();
    m_table->setColumnCount(headers.size());
    m_table->setRowCount(m_weekCount);
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setColumnWidth(0, 56);  // 周次
    for (int c = 1; c <= 7; ++c) m_table->setColumnWidth(c, 62);
    m_table->setColumnWidth(8, 260); // 备注

    for (int r = 0; r < m_weekCount; ++r) {
        // 周次
        QTableWidgetItem* weekItem = new QTableWidgetItem(weekName(r + 1));
        weekItem->setTextAlignment(Qt::AlignCenter);
        weekItem->setForeground(QBrush(Qt::white));
        m_table->setItem(r, 0, weekItem);

        for (int d = 0; d < 7; ++d) {
            const QDate date = m_termStartMonday.addDays(r * 7 + d);
            QTableWidgetItem* item = new QTableWidgetItem(formatDateCell(date));
            item->setTextAlignment(Qt::AlignCenter);
            applyCellStyle(item, date);
            m_table->setItem(r, 1 + d, item);
        }

        // 备注：按日期汇总到对应周（避免全部挤在第一行）
        // 使用 QSet 去重，避免同一周的多个日期有相同备注时重复显示
        QSet<QString> uniqueRemarks;
        for (int d = 0; d < 7; ++d) {
            const QDate date = m_termStartMonday.addDays(r * 7 + d);
            if (m_remarksByDate.contains(date)) {
                const QStringList& dateRemarks = m_remarksByDate.value(date);
                for (const QString& remark : dateRemarks) {
                    if (!remark.isEmpty()) {
                        uniqueRemarks.insert(remark);
                    }
                }
            }
        }
        // 将去重后的备注转换为列表并排序（可选，保持一致性）
        QStringList weekRemarks = uniqueRemarks.values();
        // 兼容：如果没有按日期备注，但传了整体 remarkText，则仍放第一周
        const QString remarkTextForWeek = !weekRemarks.isEmpty()
            ? weekRemarks.join("\n")
            : ((r == 0) ? m_remarkText : QString());

        QTableWidgetItem* remarkItem = new QTableWidgetItem(remarkTextForWeek);
        remarkItem->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        remarkItem->setForeground(QBrush(QColor(255, 255, 255, 220)));
        remarkItem->setFlags(remarkItem->flags() & ~Qt::ItemIsSelectable);
        m_table->setItem(r, 8, remarkItem);

        // 行高：有备注则自适应更高
        int lines = remarkTextForWeek.isEmpty() ? 1 : (remarkTextForWeek.count('\n') + 1);
        int rowH = qMax(28, 18 * lines + 10);
        m_table->setRowHeight(r, rowH);
    }
}

QString TASchoolCalendarWidget::weekName(int idx) const
{
    static const QStringList names = {
        QString::fromUtf8(u8"一"), QString::fromUtf8(u8"二"), QString::fromUtf8(u8"三"), QString::fromUtf8(u8"四"),
        QString::fromUtf8(u8"五"), QString::fromUtf8(u8"六"), QString::fromUtf8(u8"七"), QString::fromUtf8(u8"八"),
        QString::fromUtf8(u8"九"), QString::fromUtf8(u8"十"), QString::fromUtf8(u8"十一"), QString::fromUtf8(u8"十二"),
        QString::fromUtf8(u8"十三"), QString::fromUtf8(u8"十四"), QString::fromUtf8(u8"十五"), QString::fromUtf8(u8"十六"),
        QString::fromUtf8(u8"十七"), QString::fromUtf8(u8"十八"), QString::fromUtf8(u8"十九"), QString::fromUtf8(u8"二十"),
        QString::fromUtf8(u8"二十一"), QString::fromUtf8(u8"二十二")
    };
    if (idx >= 1 && idx <= names.size()) return names[idx - 1];
    return QString::number(idx);
}

QString TASchoolCalendarWidget::formatDateCell(const QDate& date) const
{
    if (!date.isValid()) return QString();
    // 起始日显示 YYYY/M/D；每月1号显示 M/D；跨年1月1号显示 YYYY/M/D
    if (date == m_termStartMonday) {
        return QString("%1/%2/%3").arg(date.year()).arg(date.month()).arg(date.day());
    }
    if (date.day() == 1) {
        // 跨年时显示年份
        if (date.year() != m_termStartMonday.year()) {
            return QString("%1/%2/%3").arg(date.year()).arg(date.month()).arg(date.day());
        }
        return QString("%1/%2").arg(date.month()).arg(date.day());
    }
    return QString::number(date.day());
}

void TASchoolCalendarWidget::applyCellStyle(QTableWidgetItem* item, const QDate& date) const
{
    if (!item || !date.isValid()) return;

    // 基础色：深色底，配白字（周一~周五工作日蓝；周末/节假日绿）
    QColor workdayColor(38, 72, 116);          // 深蓝
    QColor weekendOrHolidayColor(34, 104, 72); // 深绿

    const int dow = date.dayOfWeek(); // 1=Mon..7=Sun
    bool isWeekend = (dow == 6 || dow == 7);

    bool isHoliday = m_holidayDates.contains(date);
    bool isMakeupWork = m_makeupWorkDates.contains(date);

    QColor bg = isWeekend ? weekendOrHolidayColor : workdayColor;
    if (isHoliday) bg = weekendOrHolidayColor;
    if (isMakeupWork) bg = workdayColor;

    item->setBackground(QBrush(bg));
    item->setForeground(QBrush(Qt::white));
}

void TASchoolCalendarWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect r(0, 0, width(), height());
    QPainterPath path;
    path.addRoundedRect(r.adjusted(0, 0, -1, -1), m_radius, m_radius);
    painter.fillPath(path, QBrush(m_backgroundColor));

    QPen pen(m_borderColor);
    pen.setWidth(m_borderWidth);
    painter.strokePath(path, pen);
}

void TASchoolCalendarWidget::enterEvent(QEvent* event)
{
    QWidget::enterEvent(event);
    if (m_closeButton) m_closeButton->show();
}

void TASchoolCalendarWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    if (m_closeButton) m_closeButton->hide();
}

void TASchoolCalendarWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_closeButton) {
        const int margin = 10;
        m_closeButton->move(width() - m_closeButton->width() - margin, margin);
    }
}

void TASchoolCalendarWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!qobject_cast<QAbstractButton*>(child) && !qobject_cast<QAbstractItemView*>(child)) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TASchoolCalendarWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TASchoolCalendarWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
void TACalendarWidget::populateYearCombo()
{
    m_yearMenu->clear();

    int currentYear = m_currentDate.year();
    int startYear = currentYear;
    int endYear = currentYear + 6;

   
    for (int year = startYear; year <= endYear; ++year) {
        QAction* action = m_yearMenu->addAction(QString::number(year));
        action->setData(year);
    }

    connect(m_yearMenu, &QMenu::triggered, this, [this](QAction* action) {
        bool ok;
        int selectedYear = action->data().toInt(&ok);
        if (ok) {
            onYearChanged(selectedYear);
        }
        });
}

void TACalendarWidget::populateMonthCombo()
{
    m_monthMenu->clear();
    for (int month = 1; month <= 12; ++month) {
        QString monthName = QDate(2000, month, 1).toString("MMMM");
        QAction* action = m_monthMenu->addAction(monthName);
        action->setData(month);
    }
    connect(m_monthMenu, &QMenu::triggered, this, [this](QAction* action) {
        bool ok;
        int selectedMonth = action->data().toInt(&ok);
        if (ok) {
            onMonthChanged(selectedMonth);
        }
        });
   
}
void TACalendarWidget::onYearChanged(int year)
{
    if (year > 0) {
        m_currentDate = QDate(year, m_currentDate.month(), 1);
        updateYearButton();
        updateCalendar();
    }
}
void TACalendarWidget::onMonthChanged(int month)
{
    if (month > 0) {
        m_currentDate = QDate(m_currentDate.year(), month, 1);
        updateMonthButton();
        updateCalendar();
    }
}
void TACalendarWidget::updateYearButton()
{
    m_yearButton->setText(QString::number(m_currentDate.year())+"年");
}
void TACalendarWidget::updateMonthButton()
{
    m_monthButton->setText(QString::number(m_currentDate.month())+"月");
}
void TACalendarWidget::createWeekdayLabels()
{
    QStringList weekdayNames = {
         tr("一"),
         tr("二"),
         tr("三"),
         tr("四"),
         tr("五"),
         tr("六"),
         tr("日") 
    };
    for (int i = 0; i < 7; ++i) {
        QLabel* label = new QLabel(weekdayNames[i], this);
        label->setAlignment(Qt::AlignCenter);
        label->setProperty("isWeekdayLabel", true);
        m_calendarLayout->addWidget(label, 0, i);
    }
}

void TACalendarWidget::updateCalendar()
{
    clearDateButtons();
    QDate firstDayOfMonth(m_currentDate.year(), m_currentDate.month(), 1);
    int dayOfWeek = firstDayOfMonth.dayOfWeek();
    int startCol = dayOfWeek - 1;

    int daysInMonth = m_currentDate.daysInMonth();
    int totalCells = daysInMonth + startCol;
    int numRows = (totalCells + 6) / 7;
    int currentRow = 1;
    int currentCol = 0;
    QDate prevMonth = m_currentDate.addMonths(-1);
    int daysInPrevMonth = prevMonth.daysInMonth();
    int prevMonthStartDay = daysInPrevMonth - startCol + 1;

    for (int d = prevMonthStartDay; d <= daysInPrevMonth; ++d) {
        QPushButton* btn = new QPushButton(QString::number(d), this);
        btn->setEnabled(false);
        m_calendarLayout->addWidget(btn, currentRow, currentCol);
        m_dateButtons.append(btn);

        currentCol++;
        if (currentCol >= 7) {
            currentCol = 0;
            currentRow++;
        }
    }
    for (int d = 1; d <= daysInMonth; ++d) {
        QPushButton* btn = new QPushButton(QString::number(d), this);
        QDate dateObj(m_currentDate.year(), m_currentDate.month(), d);

        // 设置按钮的样式
        QString style = "";
        if (dateObj == QDate::currentDate()) {
            style += "background-color: lightblue; font-weight: bold;";
        }
        if (dateObj == m_selectedDate) {
            style += " border: 2px solid red;";
        }
        btn->setStyleSheet(style);
        connect(btn, &QPushButton::clicked, this, [this, dateObj]() {
            // 点击日期即可选中（会触发 selectionChanged，由外部决定是否关闭弹窗）
            this->setSelectedDate(dateObj);
            });

        m_calendarLayout->addWidget(btn, currentRow, currentCol);
        m_dateButtons.append(btn);

        currentCol++;
        if (currentCol >= 7) {
            currentCol = 0;
            currentRow++;
        }
    }

    int totalGridCells = numRows * 7;
    int remainingCells = totalGridCells - (startCol + daysInMonth);

    for (int d = 1; d <= remainingCells; ++d) {
        QPushButton* btn = new QPushButton(QString::number(d), this);
        btn->setStyleSheet("color: lightgray;");
        btn->setEnabled(false);
        m_calendarLayout->addWidget(btn, currentRow, currentCol);
        m_dateButtons.append(btn);

        currentCol++;
        if (currentCol >= 7) {
            currentCol = 0;
            currentRow++;
        }
    }

    for (int r = 1; r < numRows; ++r) {
        m_calendarLayout->setRowStretch(r, 1);
    }
}

void TACalendarWidget::clearDateButtons()
{
    qDeleteAll(m_dateButtons);
    m_dateButtons.clear();
}


void TACalendarWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);


    QRect rect(0, 0, width(), height());
    int cornerRadius = m_radius;
    QPainterPath path;
    path.addRoundedRect(rect, cornerRadius, cornerRadius);

    painter.fillPath(path, QBrush(m_backgroundColor));

    QPen pen;
    pen.setWidth(m_borderWidth);
    pen.setColor(m_borderColor);
    painter.strokePath(path, pen);
}

QDate TACalendarWidget::selectedDate() const
{
    return m_selectedDate;
}

void TACalendarWidget::setSelectedDate(const QDate& date)
{
    if (date.isValid() && date != m_selectedDate) {
        m_selectedDate = date;
        m_currentDate = date;
        // 同步顶部年/月显示
        updateYearButton();
        updateMonthButton();
        updateCalendar();
        emit selectionChanged();
    }
}
void TACalendarWidget::setRadius(int val)
{
    if (m_radius != val)
    {
        m_radius = val;
        update();
    }
}
void TACalendarWidget::setBorderWidth(int val)
{
    if (m_borderWidth != val)
    {
        m_borderWidth = val;
        update();
    }
}
void TACalendarWidget::setBorderColor(const QColor& color)
{
    if (m_borderColor != color)
    {
        m_borderColor = color;
        update();
    }
}
void TACalendarWidget::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor != color)
    {
        m_backgroundColor = color;
        update();
    }
}

void TASchoolCalendarWidget::importFromJson(const QString& jsonFilePath)
{
    QFile file(jsonFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"无法打开文件: %1").arg(jsonFilePath),
            TACalendarToastWidget::Error);
        return;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"JSON解析失败: %1").arg(parseError.errorString()),
            TACalendarToastWidget::Error);
        return;
    }

    if (!jsonDoc.isObject()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"JSON格式错误：根元素必须是对象"),
            TACalendarToastWidget::Error);
        return;
    }

    QJsonObject root = jsonDoc.object();
    
    // 解析基本信息
    QString schoolName = root["schoolName"].toString();
    QString termTitle = root["termTitle"].toString();
    
    // 解析开始日期
    QJsonObject startDateObj = root["startDate"].toObject();
    int year = startDateObj["year"].toInt();
    int month = startDateObj["month"].toInt();
    int day = startDateObj["day"].toInt();
    QDate startMonday(year, month, day);
    
    if (!startMonday.isValid()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"开始日期无效"),
            TACalendarToastWidget::Error);
        return;
    }
    
    int weekCount = root["weekCount"].toInt();
    if (weekCount <= 0) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"周数无效"),
            TACalendarToastWidget::Error);
        return;
    }

    // 解析节假日
    QSet<QDate> holidays;
    QJsonArray holidayArray = root["holidays"].toArray();
    for (const QJsonValue& value : holidayArray) {
        if (value.isObject()) {
            QJsonObject dateObj = value.toObject();
            QDate date(dateObj["year"].toInt(), dateObj["month"].toInt(), dateObj["day"].toInt());
            if (date.isValid()) {
                holidays.insert(date);
            }
        } else if (value.isString()) {
            // 支持日期范围格式 "2025-10-1:2025-10-8"
            QString dateStr = value.toString();
            if (dateStr.contains(":")) {
                QStringList parts = dateStr.split(":");
                if (parts.size() == 2) {
                    QDate startDate = QDate::fromString(parts[0], "yyyy-M-d");
                    QDate endDate = QDate::fromString(parts[1], "yyyy-M-d");
                    if (startDate.isValid() && endDate.isValid()) {
                        QSet<QDate> rangeDates = buildRangeDates(startDate, endDate);
                        holidays.unite(rangeDates);
                    }
                }
            } else {
                QDate date = QDate::fromString(dateStr, "yyyy-M-d");
                if (date.isValid()) {
                    holidays.insert(date);
                }
            }
        }
    }

    // 解析补班日期
    QSet<QDate> makeupWorkDates;
    QJsonArray makeupArray = root["makeupWorkDates"].toArray();
    for (const QJsonValue& value : makeupArray) {
        if (value.isObject()) {
            QJsonObject dateObj = value.toObject();
            QDate date(dateObj["year"].toInt(), dateObj["month"].toInt(), dateObj["day"].toInt());
            if (date.isValid()) {
                makeupWorkDates.insert(date);
            }
        } else if (value.isString()) {
            QDate date = QDate::fromString(value.toString(), "yyyy-M-d");
            if (date.isValid()) {
                makeupWorkDates.insert(date);
            }
        }
    }

    // 解析备注
    QMap<QDate, QStringList> remarksByDate;
    QJsonObject remarksObj = root["remarks"].toObject();
    for (auto it = remarksObj.begin(); it != remarksObj.end(); ++it) {
        QString dateStr = it.key();
        QDate date = QDate::fromString(dateStr, "yyyy-M-d");
        if (date.isValid()) {
            if (it.value().isArray()) {
                QJsonArray remarkArray = it.value().toArray();
                QStringList remarks;
                for (const QJsonValue& remarkValue : remarkArray) {
                    remarks << remarkValue.toString();
                }
                remarksByDate[date] = remarks;
            } else if (it.value().isString()) {
                remarksByDate[date] << it.value().toString();
            }
        }
    }

    // 更新校历
    setCalendarInfo(schoolName, termTitle, startMonday, weekCount, QString(), holidays, makeupWorkDates);
    setRemarksByDate(remarksByDate);
    
    TACalendarToastWidget::showToast(this, 
        QString::fromUtf8(u8"校历导入成功！"),
        TACalendarToastWidget::Success);
    
    // 自动上传到服务器
    uploadCalendarToServer();
}

void TASchoolCalendarWidget::uploadCalendarToServer()
{
    if (!m_httpHandler) {
        return;
    }
    
    // 检查必要数据
    if (m_schoolName.isEmpty() || m_termTitle.isEmpty() || !m_termStartMonday.isValid() || m_weekCount <= 0) {
        qWarning() << "校历数据不完整，无法上传";
        return;
    }
    
    // 获取学校唯一编号和老师唯一编号
    UserInfo userInfo = CommonInfo::GetData();
    QString schoolId = userInfo.schoolId;
    QString teacherUniqueId = userInfo.teacher_unique_id;
    
    if (schoolId.isEmpty()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"无法获取学校唯一编号，校历上传可能失败。请确保已登录并设置了学校信息。"),
            TACalendarToastWidget::Warning);
        // 继续上传，让服务器端处理
    }
    
    if (teacherUniqueId.isEmpty()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"无法获取老师唯一编号，校历上传可能失败。请确保已登录。"),
            TACalendarToastWidget::Warning);
        // 继续上传，让服务器端处理
    }
    
    // 构建JSON数据
    QJsonObject jsonObj;
    jsonObj["schoolName"] = m_schoolName;
    jsonObj["termTitle"] = m_termTitle;
    jsonObj["schoolId"] = schoolId;  // 学校唯一编号
    jsonObj["teacherUniqueId"] = teacherUniqueId;  // 老师唯一编号
    
    // 开始日期
    QJsonObject startDateObj;
    startDateObj["year"] = m_termStartMonday.year();
    startDateObj["month"] = m_termStartMonday.month();
    startDateObj["day"] = m_termStartMonday.day();
    jsonObj["startDate"] = startDateObj;
    
    jsonObj["weekCount"] = m_weekCount;
    
    // 节假日数组
    QJsonArray holidayArray;
    for (const QDate& date : m_holidayDates) {
        QJsonObject dateObj;
        dateObj["year"] = date.year();
        dateObj["month"] = date.month();
        dateObj["day"] = date.day();
        holidayArray.append(dateObj);
    }
    jsonObj["holidays"] = holidayArray;
    
    // 补班日期数组
    QJsonArray makeupArray;
    for (const QDate& date : m_makeupWorkDates) {
        QJsonObject dateObj;
        dateObj["year"] = date.year();
        dateObj["month"] = date.month();
        dateObj["day"] = date.day();
        makeupArray.append(dateObj);
    }
    jsonObj["makeupWorkDates"] = makeupArray;
    
    // 备注（按日期）
    QJsonObject remarksObj;
    for (auto it = m_remarksByDate.begin(); it != m_remarksByDate.end(); ++it) {
        QString dateStr = QString("%1-%2-%3").arg(it.key().year()).arg(it.key().month()).arg(it.key().day());
        QJsonArray remarkArray;
        for (const QString& remark : it.value()) {
            remarkArray.append(remark);
        }
        remarksObj[dateStr] = remarkArray;
    }
    jsonObj["remarks"] = remarksObj;
    
    // 转换为JSON字符串
    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);
    
    // 调试输出
    qDebug() << "========== 上传校历到服务器 ==========";
    qDebug() << "学校名称:" << m_schoolName;
    qDebug() << "学校唯一编号:" << schoolId;
    qDebug() << "老师唯一编号:" << teacherUniqueId;
    qDebug() << "学期标题:" << m_termTitle;
    qDebug() << "开始日期:" << m_termStartMonday.toString("yyyy-MM-dd");
    qDebug() << "周数:" << m_weekCount;
    qDebug() << "节假日数量:" << m_holidayDates.size();
    qDebug() << "补班日期数量:" << m_makeupWorkDates.size();
    qDebug() << "备注数量:" << m_remarksByDate.size();
    qDebug() << "JSON数据:" << QString::fromUtf8(jsonData);
    
    // 上传到服务器
    QString uploadUrl = QString("%1/api/calendar/upload").arg("http://47.100.126.194:5000");
    qDebug() << "上传URL:" << uploadUrl;
    m_httpHandler->post(uploadUrl, jsonData);
}

void TASchoolCalendarWidget::loadCalendarFromServer(const QString& termTitle, const QString& startDate)
{
    if (!m_httpHandlerForGet) {
        return;
    }
    
    // 获取学校唯一编号和老师唯一编号
    UserInfo userInfo = CommonInfo::GetData();
    QString schoolId = userInfo.schoolId;
    QString teacherUniqueId = userInfo.teacher_unique_id;
    
    if (schoolId.isEmpty() || teacherUniqueId.isEmpty()) {
        qWarning() << "无法获取学校编号或老师编号，无法加载校历";
        return;
    }
    
    // 构建请求URL
    QString baseUrl = "http://47.100.126.194:5000/api/calendar/get";
    QUrl url(baseUrl);
    QUrlQuery query;
    query.addQueryItem("school_id", schoolId);
    query.addQueryItem("teacher_unique_id", teacherUniqueId);
    
    if (!termTitle.isEmpty()) {
        query.addQueryItem("term_title", termTitle);
    }
    if (!startDate.isEmpty()) {
        query.addQueryItem("start_date", startDate);
    }
    
    url.setQuery(query);
    
    qDebug() << "========== 从服务器加载校历 ==========";
    qDebug() << "学校编号:" << schoolId;
    qDebug() << "老师编号:" << teacherUniqueId;
    qDebug() << "请求URL:" << url.toString();
    
    // 发送GET请求
    m_httpHandlerForGet->get(url.toString());
}

void TASchoolCalendarWidget::parseCalendarResponse(const QString& response)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8());
    if (!jsonDoc.isObject()) {
        qWarning() << "校历响应格式错误";
        return;
    }
    
    QJsonObject root = jsonDoc.object();
    int code = root["code"].toInt(0);
    QString message = root["message"].toString();
    
    if (code != 200) {
        qWarning() << "获取校历失败:" << message;
        return;
    }
    
    QJsonValue dataValue = root["data"];
    
    // 判断是单个校历还是多个校历
    if (dataValue.isObject()) {
        // 单个校历
        parseSingleCalendar(dataValue.toObject());
    } else if (dataValue.isArray()) {
        // 多个校历，取第一个（或最新的）
        QJsonArray calendarArray = dataValue.toArray();
        if (calendarArray.size() > 0) {
            parseSingleCalendar(calendarArray[0].toObject());
        }
    }
}

void TASchoolCalendarWidget::parseSingleCalendar(const QJsonObject& calendarObj)
{
    // 解析基本信息
    QString schoolName = calendarObj["schoolName"].toString();
    QString termTitle = calendarObj["termTitle"].toString();
    
    // 解析开始日期
    QJsonObject startDateObj = calendarObj["startDate"].toObject();
    int year = startDateObj["year"].toInt();
    int month = startDateObj["month"].toInt();
    int day = startDateObj["day"].toInt();
    QDate startMonday(year, month, day);
    
    if (!startMonday.isValid()) {
        qWarning() << "开始日期无效";
        return;
    }
    
    int weekCount = calendarObj["weekCount"].toInt();
    if (weekCount <= 0) {
        qWarning() << "周数无效";
        return;
    }
    
    // 解析节假日
    QSet<QDate> holidays;
    QJsonArray holidayArray = calendarObj["holidays"].toArray();
    for (const QJsonValue& value : holidayArray) {
        if (value.isObject()) {
            QJsonObject dateObj = value.toObject();
            QDate date(dateObj["year"].toInt(), dateObj["month"].toInt(), dateObj["day"].toInt());
            if (date.isValid()) {
                holidays.insert(date);
            }
        }
    }
    
    // 解析补班日期
    QSet<QDate> makeupWorkDates;
    QJsonArray makeupArray = calendarObj["makeupWorkDates"].toArray();
    for (const QJsonValue& value : makeupArray) {
        if (value.isObject()) {
            QJsonObject dateObj = value.toObject();
            QDate date(dateObj["year"].toInt(), dateObj["month"].toInt(), dateObj["day"].toInt());
            if (date.isValid()) {
                makeupWorkDates.insert(date);
            }
        }
    }
    
    // 解析备注
    QMap<QDate, QStringList> remarksByDate;
    QJsonObject remarksObj = calendarObj["remarks"].toObject();
    for (auto it = remarksObj.begin(); it != remarksObj.end(); ++it) {
        QString dateStr = it.key();
        QDate date = QDate::fromString(dateStr, "yyyy-M-d");
        if (!date.isValid()) {
            // 尝试其他格式
            date = QDate::fromString(dateStr, "yyyy-MM-dd");
        }
        if (date.isValid()) {
            if (it.value().isArray()) {
                QJsonArray remarkArray = it.value().toArray();
                QStringList remarks;
                for (const QJsonValue& remarkValue : remarkArray) {
                    remarks << remarkValue.toString();
                }
                remarksByDate[date] = remarks;
            } else if (it.value().isString()) {
                remarksByDate[date] << it.value().toString();
            }
        }
    }
    
    // 更新校历界面
    setCalendarInfo(schoolName, termTitle, startMonday, weekCount, QString(), holidays, makeupWorkDates);
    setRemarksByDate(remarksByDate);
    
    qDebug() << "校历加载成功:" << schoolName << termTitle;
}

// 辅助函数：解析日期字符串
static QDate parseDate(const QString& dateStr)
{
    if (dateStr.isEmpty()) return QDate();
    
    // 尝试多种日期格式
    QDate date = QDate::fromString(dateStr, "yyyy-M-d");
    if (!date.isValid()) {
        date = QDate::fromString(dateStr, "yyyy/MM/dd");
    }
    if (!date.isValid()) {
        date = QDate::fromString(dateStr, "yyyy-MM-dd");
    }
    if (!date.isValid()) {
        date = QDate::fromString(dateStr, "yyyy/M/d");
    }
    if (!date.isValid()) {
        date = QDate::fromString(dateStr, "M/d/yyyy");
    }
    if (!date.isValid()) {
        date = QDate::fromString(dateStr, "MM/dd/yyyy");
    }
    
    return date;
}

// 辅助函数：从日期单元格解析日期
// cellValue: 单元格显示值（可能是"3"、"15"、"2025-9-1"、"12月1日"等）
// currentYear: 当前年份（用于推断只有日期的单元格）
// currentMonth: 当前月份（用于推断只有日期的单元格）
// isFirstDayOfTerm: 是否是学期第一天
// isLastDayOfTerm: 是否是学期最后一天
static QDate parseDateFromCell(const QString& cellValue, int currentYear, int currentMonth, 
                               bool isFirstDayOfTerm, bool isLastDayOfTerm, bool isFirstDayOfYear)
{
    if (cellValue.isEmpty()) return QDate();
    
    QString value = cellValue.trimmed();
    
    // 格式1: 完整日期 yyyy-m-d (学期开始/结束、每年第一天)
    if (value.contains("-") && value.length() >= 8) {
        QDate date = QDate::fromString(value, "yyyy-M-d");
        if (!date.isValid()) {
            date = QDate::fromString(value, "yyyy-MM-dd");
        }
        if (date.isValid()) {
            return date;
        }
    }
    
    // 格式2: "m月d日" (每月第一天，但不是学期开始/结束和每年第一天)
    if (value.contains(QString::fromUtf8(u8"月")) && value.contains(QString::fromUtf8(u8"日"))) {
        // 提取月份和日期
        int monthPos = value.indexOf(QString::fromUtf8(u8"月"));
        int dayPos = value.indexOf(QString::fromUtf8(u8"日"));
        if (monthPos > 0 && dayPos > monthPos) {
            bool ok1, ok2;
            int month = value.left(monthPos).toInt(&ok1);
            int day = value.mid(monthPos + 1, dayPos - monthPos - 1).toInt(&ok2);
            if (ok1 && ok2 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                QDate date(currentYear, month, day);
                if (date.isValid()) {
                    return date;
                }
            }
        }
    }
    
    // 格式3: 只有日期数字 (普通日期)
    bool ok;
    int day = value.toInt(&ok);
    if (ok && day >= 1 && day <= 31) {
        QDate date(currentYear, currentMonth, day);
        if (date.isValid()) {
            return date;
        }
    }
    
    return QDate();
}

void TASchoolCalendarWidget::importFromExcel(const QString& excelFilePath)
{
    using namespace QXlsx;
    
    Document xlsx(excelFilePath);
    if (!xlsx.load()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"无法打开Excel文件: %1").arg(excelFilePath),
            TACalendarToastWidget::Error);
        return;
    }
    
    // 获取第一个工作表
    QStringList sheetNames = xlsx.sheetNames();
    if (sheetNames.isEmpty()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"Excel文件没有工作表"),
            TACalendarToastWidget::Error);
        return;
    }
    
    xlsx.selectSheet(sheetNames.first());
    CellRange range = xlsx.dimension();
    if (range.rowCount() == 0 || range.columnCount() == 0) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"Excel文件为空"),
            TACalendarToastWidget::Error);
        return;
    }
    
    // 解析Excel数据
    QString schoolName;
    QString termTitle;
    QDate startMonday;
    QDate endDate;
    int weekCount = 0;
    QSet<QDate> holidays;
    QSet<QDate> makeupWorkDates;
    QMap<QDate, QStringList> remarksByDate;
    
    // 按照标准格式解析：
    // 第一行：校历说明（学校名、学年、学期）
    // 第二行：表头（星期、周次、星期一到星期天、备注）
    // 第三行开始：数据行（周次、日期、备注）
    // 最后一行：说明文字
    
    // 解析第一行：校历说明
    Cell* firstRowCell = xlsx.cellAt(1, 1);
    if (firstRowCell) {
        QString firstRowText = firstRowCell->value().toString().trimmed();
        // 尝试从第一行提取学校名和学期信息
        // 格式示例："费县第五中学 2025～2026学年度第一学期校历"
        if (firstRowText.contains(QString::fromUtf8(u8"学年度"))) {
            // 提取学校名（在"学年度"之前的部分）
            int pos = firstRowText.indexOf(QString::fromUtf8(u8"学年度"));
            if (pos > 0) {
                // 查找学校名（通常在"学年度"之前的数字年份之前）
                QString beforeTerm = firstRowText.left(pos);
                // 查找最后一个空格或数字之前的部分作为学校名
                int lastSpace = beforeTerm.lastIndexOf(" ");
                if (lastSpace > 0) {
                    schoolName = beforeTerm.left(lastSpace).trimmed();
                } else {
                    // 如果没有空格，尝试查找数字位置
                    QRegExp yearPattern("\\d{4}");
                    int yearPos = beforeTerm.indexOf(yearPattern);
                    if (yearPos > 0) {
                        schoolName = beforeTerm.left(yearPos).trimmed();
                    } else {
                        schoolName = beforeTerm.trimmed();
                    }
                }
            }
            // 提取学期信息（包含"学年度"和"学期"的部分）
            termTitle = firstRowText;
        } else {
            // 如果没有标准格式，直接使用第一行作为标题
            termTitle = firstRowText;
            // 尝试提取学校名（如果有空格分隔）
            QStringList parts = firstRowText.split(" ");
            if (parts.size() > 0) {
                schoolName = parts[0];
            }
        }
    }
    
    // 验证第二行是否为表头
    // 第二行第一列应该是"星期/周次"（可能是合并单元格或包含两个词）
    // 第二列开始应该是"一"、"二"、"三"等（星期一到星期天），最后一列是"备注"
    bool isValidHeader = false;
    if (range.rowCount() >= 2) {
        Cell* headerCell1 = xlsx.cellAt(2, 1); // 星期/周次
        if (headerCell1) {
            QString h1 = headerCell1->value().toString().trimmed();
            // 检查第一列是否包含"星期"和"周次"（可能是合并单元格，包含两个词）
            bool hasWeekday = h1.contains(QString::fromUtf8(u8"星期"));
            bool hasWeekNum = h1.contains(QString::fromUtf8(u8"周次"));
            
            // 检查第二列是否是"一"（星期一）
            Cell* headerCell2 = xlsx.cellAt(2, 2);
            bool hasMonday = false;
            if (headerCell2) {
                QString h2 = headerCell2->value().toString().trimmed();
                hasMonday = (h2 == QString::fromUtf8(u8"一"));
            }
            
            // 如果第一列包含"星期"和"周次"，且第二列是"一"，则认为是有效表头
            if ((hasWeekday && hasWeekNum) && hasMonday) {
                isValidHeader = true;
            }
            // 或者第一列包含"星期"或"周次"，且第二列是"一"，也认为是有效表头
            else if ((hasWeekday || hasWeekNum) && hasMonday) {
                isValidHeader = true;
            }
        }
    }
    
    if (!isValidHeader) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"Excel文件格式不正确：第二行应该是表头（第一列：星期/周次，第二列开始：星期一到星期天，最后一列：备注）"),
            TACalendarToastWidget::Error);
        return;
    }
    
    // 从第三行开始解析数据
    int dataStartRow = 3;
    int lastDataRow = range.rowCount();
    
    // 查找最后一行（包含"注："的行）
    for (int row = range.rowCount(); row >= dataStartRow; --row) {
        Cell* cell = xlsx.cellAt(row, 1);
        if (cell) {
            QString text = cell->value().toString().trimmed();
            if (text.contains(QString::fromUtf8(u8"注：")) || text.contains(QString::fromUtf8(u8"注:"))) {
                lastDataRow = row - 1;
                break;
            }
        }
    }
    
    if (lastDataRow < dataStartRow) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"Excel文件格式不正确：没有找到数据行"),
            TACalendarToastWidget::Error);
        return;
    }
    
    weekCount = lastDataRow - dataStartRow + 1;
    
    // 首先解析第一行第一列（学期开始日期）
    Cell* firstDateCell = xlsx.cellAt(dataStartRow, 2);
    if (firstDateCell) {
        QDate firstDate;
        QVariant cellValue = firstDateCell->value();
        
        // 首先检查单元格是否是日期类型（Excel日期可能存储为Double类型）
        if (firstDateCell->isDateTime()) {
            // 使用 dateTime() 方法获取日期时间值
            QVariant dateTimeValue = firstDateCell->dateTime();
            if (dateTimeValue.type() == QVariant::Date) {
                firstDate = dateTimeValue.toDate();
            } else if (dateTimeValue.type() == QVariant::DateTime) {
                firstDate = dateTimeValue.toDateTime().date();
            }
        }
        // 如果 isDateTime() 返回 false，检查 QVariant 类型
        else if (cellValue.type() == QVariant::Date) {
            // 如果是日期类型，直接使用
            firstDate = cellValue.toDate();
        } else if (cellValue.type() == QVariant::DateTime) {
            // 如果是日期时间类型，提取日期部分
            firstDate = cellValue.toDateTime().date();
        } else if (cellValue.type() == QVariant::Double) {
            // 如果是 Double 类型，可能是日期数字，尝试检查格式
            Format cellFormat = firstDateCell->format();
            if (cellFormat.isValid() && cellFormat.isDateTimeFormat()) {
                // 格式是日期格式，尝试转换
                // 注意：QXlsx 可能已经处理了，但如果没有，我们需要手动转换
                // 这里先尝试使用 dateTime() 方法
                QVariant dateTimeValue = firstDateCell->dateTime();
                if (dateTimeValue.isValid()) {
                    if (dateTimeValue.type() == QVariant::Date) {
                        firstDate = dateTimeValue.toDate();
                    } else if (dateTimeValue.type() == QVariant::DateTime) {
                        firstDate = dateTimeValue.toDateTime().date();
                    }
                }
            }
        }
        
        // 如果以上方法都失败，尝试作为字符串解析
        if (!firstDate.isValid()) {
            QString firstDateStr = cellValue.toString().trimmed();
            // 优先尝试 yyyy-m-d 格式（如"2025-9-1"）
            firstDate = QDate::fromString(firstDateStr, "yyyy-M-d");
            if (!firstDate.isValid()) {
                // 如果失败，尝试其他格式
                firstDate = parseDate(firstDateStr);
            }
        }
        
        if (firstDate.isValid()) {
            // 确保是周一
            int dayOfWeek = firstDate.dayOfWeek();
            if (dayOfWeek != 1) {
                int daysToMonday = (1 - dayOfWeek + 7) % 7;
                if (daysToMonday == 0) daysToMonday = 7;
                startMonday = firstDate.addDays(-daysToMonday);
            } else {
                startMonday = firstDate;
            }
        }
    }
    
    if (!startMonday.isValid()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"无法解析学期开始日期，请确保第一行第一列包含日期（格式：2025-9-1）"),
            TACalendarToastWidget::Error);
        return;
    }
    
    // 解析数据行，提取日期和节假日信息
    QDate currentDate = startMonday;
    int currentYear = startMonday.year();
    int currentMonth = startMonday.month();
    
    for (int row = dataStartRow; row <= lastDataRow; ++row) {
        // 第一列：周次（跳过）
        // 第二列到第八列：星期一到星期天的日期
        // 第九列：备注
        
        QDate weekStartDate = currentDate; // 本周的周一日期
        
        for (int col = 2; col <= 8; ++col) {
            Cell* cell = xlsx.cellAt(row, col);
            if (!cell) {
                // 如果单元格为空，根据位置推断日期
                currentDate = currentDate.addDays(1);
                continue;
            }
            
            QVariant cellValueVariant = cell->value();
            QDate date;
            
            // 判断是否是学期第一天（第一行第一列）
            bool isFirstDayOfTerm = (row == dataStartRow && col == 2);
            // 判断是否是学期最后一天（最后一行最后一列）
            bool isLastDayOfTerm = (row == lastDataRow && col == 8);
            
            // 首先检查单元格是否是日期类型（Excel日期可能存储为Double类型）
            if (cell->isDateTime()) {
                // 使用 dateTime() 方法获取日期时间值
                QVariant dateTimeValue = cell->dateTime();
                if (dateTimeValue.type() == QVariant::Date) {
                    date = dateTimeValue.toDate();
                } else if (dateTimeValue.type() == QVariant::DateTime) {
                    date = dateTimeValue.toDateTime().date();
                }
            }
            // 如果 isDateTime() 返回 false，检查 QVariant 类型
            else if (cellValueVariant.type() == QVariant::Date) {
                // 如果是日期类型，直接使用
                date = cellValueVariant.toDate();
            } else if (cellValueVariant.type() == QVariant::DateTime) {
                // 如果是日期时间类型，提取日期部分
                date = cellValueVariant.toDateTime().date();
            } else if (cellValueVariant.type() == QVariant::Double) {
                // 如果是 Double 类型，可能是日期数字，尝试检查格式
                Format cellFormat = cell->format();
                if (cellFormat.isValid() && cellFormat.isDateTimeFormat()) {
                    // 格式是日期格式，尝试使用 dateTime() 方法
                    QVariant dateTimeValue = cell->dateTime();
                    if (dateTimeValue.isValid()) {
                        if (dateTimeValue.type() == QVariant::Date) {
                            date = dateTimeValue.toDate();
                        } else if (dateTimeValue.type() == QVariant::DateTime) {
                            date = dateTimeValue.toDateTime().date();
                        }
                    }
                }
            }
            
            // 如果以上方法都失败，尝试作为字符串解析
            if (!date.isValid()) {
                QString cellValue = cellValueVariant.toString().trimmed();
                
                if (!cellValue.isEmpty()) {
                    // 格式1: 完整日期 yyyy-m-d (学期开始/结束、每年第一天)
                    if (cellValue.contains("-") && cellValue.length() >= 8) {
                        // 优先尝试 yyyy-m-d 格式（如"2025-9-1"）
                        date = QDate::fromString(cellValue, "yyyy-M-d");
                        if (!date.isValid()) {
                            date = QDate::fromString(cellValue, "yyyy-MM-dd");
                        }
                    }
                    // 格式2: "m月d日" (每月第一天)
                    else if (cellValue.contains(QString::fromUtf8(u8"月")) && cellValue.contains(QString::fromUtf8(u8"日"))) {
                        int monthPos = cellValue.indexOf(QString::fromUtf8(u8"月"));
                        int dayPos = cellValue.indexOf(QString::fromUtf8(u8"日"));
                        if (monthPos > 0 && dayPos > monthPos) {
                            bool ok1, ok2;
                            int month = cellValue.left(monthPos).toInt(&ok1);
                            int day = cellValue.mid(monthPos + 1, dayPos - monthPos - 1).toInt(&ok2);
                            if (ok1 && ok2 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
                                // 判断年份（如果月份小于当前月份，可能是下一年）
                                int year = currentYear;
                                if (month < currentMonth) {
                                    year = currentYear + 1;
                                }
                                date = QDate(year, month, day);
                            }
                        }
                    }
                    // 格式3: 只有日期数字
                    else {
                        bool ok;
                        int day = cellValue.toInt(&ok);
                        if (ok && day >= 1 && day <= 31) {
                            // 使用当前年月
                            date = QDate(currentYear, currentMonth, day);
                            // 如果日期无效或小于当前日期，可能是下一个月
                            if (!date.isValid() || date < currentDate) {
                                QDate nextMonth = QDate(currentYear, currentMonth, 1).addMonths(1);
                                date = QDate(nextMonth.year(), nextMonth.month(), day);
                            }
                        }
                    }
                }
            }
            
            // 如果解析失败，使用推断的日期
            if (!date.isValid()) {
                date = currentDate;
            }
            
            // 更新当前日期和年月
            currentDate = date;
            currentYear = date.year();
            currentMonth = date.month();
            
            // 记录学期结束日期
            if (row == lastDataRow && col == 8) {
                endDate = date;
            }
            
            // 根据单元格背景色判断是否为节假日
            // 绿色表示节假日，蓝色/橘色表示工作日
            Format cellFormat = cell->format();
            if (cellFormat.isValid()) {
                // 尝试获取背景色
                QColor bgColor = cellFormat.patternBackgroundColor();
                if (bgColor.isValid()) {
                    // 判断是否为绿色（节假日）
                    // 绿色通常RGB值中G分量较大，且R和B分量较小
                    int r = bgColor.red();
                    int g = bgColor.green();
                    int b = bgColor.blue();
                    
                    // 绿色判断：G > R 且 G > B，且G值较大
                    if (g > r && g > b && g > 100) {
                        holidays.insert(date);
                    }
                    // 蓝色判断：B > R 且 B > G（工作日，不添加为节假日）
                    // 橘色判断：R > G 且 B较小（工作日，不添加为节假日）
                }
            }
            
            // 判断是否是周末（周六或周日）
            int dayOfWeek = date.dayOfWeek();
            if (dayOfWeek == 6 || dayOfWeek == 7) {
                // 周末默认是节假日（除非是补班）
                if (!makeupWorkDates.contains(date)) {
                    holidays.insert(date);
                }
            }
        }
        
        // 读取备注列（第9列）
        Cell* remarkCell = xlsx.cellAt(row, 9);
        if (remarkCell) {
            QString remarkText = remarkCell->value().toString().trimmed();
            if (!remarkText.isEmpty()) {
                // 将备注只添加到本周的周一（weekStartDate），避免重复
                // 在显示时会自动汇总到整周
                if (weekStartDate.isValid()) {
                    QStringList& dateRemarks = remarksByDate[weekStartDate];
                    // 检查备注是否已存在，避免重复添加
                    if (!dateRemarks.contains(remarkText)) {
                        dateRemarks << remarkText;
                    }
                }
            }
        }
        
        // 移动到下一周（周一）
        currentDate = weekStartDate.addDays(7);
    }
    
    
    // 验证必要数据
    if (schoolName.isEmpty()) {
        schoolName = QString::fromUtf8(u8"未知学校");
    }
    if (termTitle.isEmpty()) {
        termTitle = QString::fromUtf8(u8"未知学期");
    }
    if (!startMonday.isValid()) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"无法从Excel文件中解析开始日期，请确保文件格式正确"),
            TACalendarToastWidget::Error);
        return;
    }
    if (weekCount <= 0) {
        TACalendarToastWidget::showToast(this, 
            QString::fromUtf8(u8"无法从Excel文件中解析周数，请确保文件包含数据行"),
            TACalendarToastWidget::Error);
        return;
    }
    
    // 更新校历
    setCalendarInfo(schoolName, termTitle, startMonday, weekCount, QString(), holidays, makeupWorkDates);
    setRemarksByDate(remarksByDate);
    
    TACalendarToastWidget::showToast(this, 
        QString::fromUtf8(u8"校历导入成功！"),
        TACalendarToastWidget::Success);
    
    // 自动上传到服务器
    uploadCalendarToServer();
}