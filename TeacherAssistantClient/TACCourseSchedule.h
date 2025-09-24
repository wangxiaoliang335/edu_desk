#pragma once
#include "TAFloatingWidget.h"
#include <QPainter>
#include <QPixmap>
#include <QPaintEvent>
#include <QLabel>
#include <QList>
#include <QHBoxLayout>
#include <QPointer>
#include <QTimer>
#include <QTime>
#include <QMap>
#include <QPair>
using namespace std;



using TimeRange = QPair<QTime, QTime>;
class TACCourseSchedule : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACCourseSchedule(QWidget* parent);
	~TACCourseSchedule();
	void updateClass(const QMap<TimeRange, QString> &classMap);
protected:
	void initShow() override;
	void cleanLayout(QLayout* layout);
private slots:
	void updateTimeSlots();
private:
	struct TimeSlot {
		QPushButton* button;
		QTime start;
		QTime end;
	};
	QVector<TimeSlot> timeSlots;
	QPointer<QVBoxLayout> contentLayout;
	QPointer<QVBoxLayout> classLayout;
	QPointer<QTimer> timer;
};
