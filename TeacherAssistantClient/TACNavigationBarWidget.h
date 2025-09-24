#pragma once

#include "TAFloatingWidget.h"
#include "common.h"
enum TACNavigationBarWidgetType
{
	FOLDER = 0,
	MESSAGE,
	IM,
	HOMEWORK,
	PREPARE_CLASS,
	USER,
	WALLPAPER,
	CALENDAR,
	TIMER
};
class TACNavigationBarWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACNavigationBarWidget(QWidget *parent);
	~TACNavigationBarWidget();
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

signals:
	void navType(TACNavigationBarWidgetType type,bool checked=false);
private:
	void initShow();

private:
	QString m_className;
};
