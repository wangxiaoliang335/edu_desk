#pragma once

#include "TADialog.h"
#include "TACalendarDialog.h"
#include <QLineEdit>
class TACCountDownDialog  : public TADialog
{
	Q_OBJECT

public:
	TACCountDownDialog(QWidget *parent);
	~TACCountDownDialog();
	int daysLeft();
	QString content();
protected:
	void showEvent(QShowEvent* event) override;
private:
	QLabel* countDownLabel;
	QLineEdit* contentLineEdit;

};
