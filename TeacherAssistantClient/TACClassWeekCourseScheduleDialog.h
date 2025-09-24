#pragma once

#include "TABaseDialog.h"
#include <QHBoxLayout>
#include <QGridLayout>
#include "TACToolWidget.h"
class TACClassWeekCourseScheduleDialog  : public TABaseDialog
{
	Q_OBJECT

public:
	TACClassWeekCourseScheduleDialog(QWidget *parent);
	~TACClassWeekCourseScheduleDialog();
	void updateClassList();

public slots:
	void classClick();
private:
	void init();
private:
	QGridLayout* gridLayout;
	QPushButton* currentClassButton;
	QVector<QVector<QPushButton*>> classVecotr;
};
