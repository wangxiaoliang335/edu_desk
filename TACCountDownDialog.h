#pragma once

#include "TADialog.h"
#include "TACalendarDialog.h"
#include <QLineEdit>
#include <QCloseEvent>
class TACCountDownDialog  : public TADialog
{
	Q_OBJECT

public:
	TACCountDownDialog(QWidget *parent);
	~TACCountDownDialog();
	int daysLeft();
	QString content();
	QDate date();

protected:
	void showEvent(QShowEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
private:
	QLabel* countDownLabel;
	QLineEdit* contentLineEdit;
	QDate m_date;
	TACalendarWidget* calendar;
};
