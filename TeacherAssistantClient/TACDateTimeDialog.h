#pragma once

#include "TADialog.h"
#include <QTimer>
class TACDateTimeDialog : public TADialog
{
	Q_OBJECT

public:
	TACDateTimeDialog(QWidget *parent);
	~TACDateTimeDialog();
signals:
	void updateType(int type);

private:
	QHBoxLayout* typeLayout;
	QWidget* contentWidget;
	QLabel* upContentLabel;
	QLabel* downContentLabel;
	QTimer* timer;
};
