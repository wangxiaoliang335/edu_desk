#pragma once

#include "TAFloatingWidget.h"

class TACDesktopManagerWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACDesktopManagerWidget(QWidget *parent);
	~TACDesktopManagerWidget();
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();

private:
	class TASchoolCalendarWidget* m_schoolCalendarWidget = nullptr;
	class TACTeacherCourseScheduleWindow* m_teacherCourseScheduleWindow = nullptr; // 教师课程表窗口（mainwindow.cpp 的实现）
	class SchoolCourseScheduleDialog* m_schoolCourseScheduleDialog = nullptr; // 学校/年级课程表总览
};
