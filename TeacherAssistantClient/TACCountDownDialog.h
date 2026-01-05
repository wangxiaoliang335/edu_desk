#pragma once

#include "TADialog.h"
#include "TACalendarDialog.h"
#include <QLineEdit>
#include <QDate>
#include <QCloseEvent>
class TACCountDownDialog  : public TADialog
{
	Q_OBJECT

public:
	TACCountDownDialog(QWidget *parent);
	~TACCountDownDialog();
	int daysLeft();
	QString content();
    QDate date() const { return m_date; }

signals:
    void done();
protected:
	void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
private:
	QLabel* countDownLabel;
	QLineEdit* contentLineEdit;
    QDate m_date;
    TACalendarWidget* calendar = nullptr;
    QPushButton* calendarButton = nullptr;
};
