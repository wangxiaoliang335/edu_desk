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
	class TACClassWeekCourseScheduleDialog* m_teacherCourseScheduleDlg = nullptr; // 教师课程表窗口
};
