#pragma execution_character_set("utf-8")
#include "TACalendarDialog.h"
#include <QPainterPath>
#include <QVariant>
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
            //selectDate(dateObj);
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