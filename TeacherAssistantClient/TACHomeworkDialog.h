#pragma once

#include "TABaseDialog.h"

class TACHomeworkDialog  : public TABaseDialog
{
	Q_OBJECT

public:
	TACHomeworkDialog(QWidget *parent);
	~TACHomeworkDialog();
	void setContent(const QString& text);

private:
	QLabel* contentLabel;
};
