#pragma once

#include <QDialog>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QResizeEvent>
#include "TAFloatingWidget.h"
#include "TACNavigationBarWidget.h"
#include "TACCountDownWidget.h"
#include "TACDateTimeDialog.h"
#include "TACDateTimeWidget.h"
#include "TACLogoWidget.h"
#include "TACLogoDialog.h"
#include "TACFolderWidget.h"
#include "TACCourseSchedule.h"
#include "TACFolderDialog.h"
#include "TACCountDownDialog.h"
#include "TACWallpaperLibraryDialog.h"
#include "TACHomeworkDialog.h"
#include "TACIMDialog.h"
#include "TACDesktopManagerWidget.h"
#include "TACPrepareClassDialog.h"
#include "TACClassWeekCourseScheduleDialog.h"
#include "ui_TACMainDialog.h"


class TACMainDialog : public QDialog
{
	Q_OBJECT

public:
	TACMainDialog(QWidget *parent = nullptr);
	~TACMainDialog();
	void Init();
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	void updateBackground(const QString& fileName);
private:
	Ui::TACMainDialogClass ui;
	QPixmap m_background;
	QPointer<TACNavigationBarWidget> navBarWidget;
	QPointer<TACCountDownWidget> countDownWidget;
	QPointer<TACCountDownDialog> countDownDialog;
	QPointer<TACDateTimeDialog> datetimeDialog;
	QPointer<TACDateTimeWidget> datetimeWidget;
	QPointer<TACLogoWidget> logoWidget;
	QPointer<TACLogoDialog> logoDialog;
	QPointer<TACSchoolLabelWidget> schoolLabelWidget;
	QPointer<TACClassLabelWidget> classLabelWidget;
	QPointer<TACFolderWidget> folderWidget;
	QPointer<TACCourseSchedule> courseSchedule;
	QPointer<TACFolderDialog> folderDialog;
	QPointer<TACWallpaperLibraryDialog> wallpaperLibraryDialog;
	QPointer<TACHomeworkDialog> homeworkDialog;
	QPointer<TACIMDialog> imDialog;
	QPointer<TACDesktopManagerWidget> desktopManagerWidget;
	QPointer<TACPrepareClassDialog> prepareClassDialog;
	QPointer<TACClassWeekCourseScheduleDialog> classWeekCourseScheduldDialog;
};
