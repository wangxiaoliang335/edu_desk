#pragma execution_character_set("utf-8")
#include "TACCourseSchedule.h"
#include <QPushButton>
#include <QLabel>
#include "common.h"
uint qHash(const QPair<QTime, QTime>& pair, uint seed = 0) {
    return ::qHash(qMakePair(pair.first.msecsSinceStartOfDay(), pair.second.msecsSinceStartOfDay()), seed);
}
TACCourseSchedule::TACCourseSchedule(QWidget* parent) : TAFloatingWidget(parent)
{
    this->setObjectName("TACCourseSchedule");

    this->resize(QSize(200, 603 /*820*/));
    this->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    this->setBorderColor(WIDGET_BORDER_COLOR);
    this->setBorderWidth(WIDGET_BORDER_WIDTH);
    this->setRadius(30);

    contentLayout = new QVBoxLayout();
    this->contentLayout->setSpacing(10);
    this->setLayout(this->contentLayout);
    QLabel* label = new QLabel("课表");
    label->setAlignment(Qt::AlignCenter);
    this->contentLayout->addWidget(label);
    this->classLayout = new QVBoxLayout();
    this->classLayout->setSpacing(10);
    this->contentLayout->addLayout(this->classLayout);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &TACCourseSchedule::updateTimeSlots);
    //timer->start(60 * 1000);
}

TACCourseSchedule::~TACCourseSchedule()
{
}

void TACCourseSchedule::updateClass(const QMap<TimeRange, QString>& classMap)
{
    cleanLayout(this->classLayout);
    timeSlots.clear();
    for (auto it = classMap.begin(); it != classMap.end(); ++it) {
        const TimeRange& range = it.key();
        const QString& label = it.value();

        QPushButton* classBtn = new QPushButton(label);
        this->classLayout->addWidget(classBtn);
        timeSlots.append({ classBtn, range.first, range.second });
    }
    this->classLayout->addStretch();
}

void TACCourseSchedule::initShow()
{
    QRect rect = this->getScreenGeometryWithTaskbar();
    if (rect.isEmpty())
        return;
    int x = rect.x()+rect.width()-this->width()-30;
    int y = rect.y()+200;
    this->move(x, y);
}

void TACCourseSchedule::cleanLayout(QLayout *layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QLayout* childLayout = item->layout()) {
            cleanLayout(childLayout);
            delete childLayout;
        }
        if (QWidget* widget = item->widget()) {

            delete widget;
        }
        delete item;
    }
}
void TACCourseSchedule::updateTimeSlots()
{
    QTime now = QTime::currentTime();

    for (const auto& slot : qAsConst(timeSlots)) {
        bool inRange = false;

        if (slot.start < slot.end) {
            inRange = (now >= slot.start && now < slot.end);
        }
        else {
            inRange = (now >= slot.start) || (now < slot.end);
        }

        if (inRange) {
            
        }
        else {
            //slot.button
        }
    }
}
