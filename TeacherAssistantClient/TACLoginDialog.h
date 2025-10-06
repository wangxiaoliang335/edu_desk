#pragma once

#include "TADialog.h"
#include <QTimer>
class TACLoginDialog : public TADialog
{
	Q_OBJECT

public:
	TACLoginDialog(QWidget *parent = NULL);
	~TACLoginDialog();
signals:
	void updateType(int type);

private:
	QHBoxLayout* typeLayout;
	QWidget* contentWidget;
	QLabel* upContentLabel;
	QLabel* downContentLabel;
	QTimer* timer;
};
