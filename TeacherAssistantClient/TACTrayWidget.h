#pragma once

#include "TAFloatingWidget.h"

class TACTrayWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACTrayWidget(QWidget *parent);
	~TACTrayWidget();
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
signals:
	void navType(bool checked = false);
	void navChoolInfo(bool checked = false);
};
