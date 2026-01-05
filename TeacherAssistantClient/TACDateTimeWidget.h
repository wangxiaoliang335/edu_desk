#pragma once

#include "TAFloatingWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QPushButton>

class TACDateTimeWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACDateTimeWidget(QWidget *parent);
	~TACDateTimeWidget();
	//00000011,00000001,00000010
	void setType(int type);
protected:
	void initShow() override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	
signals:
	void doubleClicked();
	
private:
	
	QLabel* upContentLabel;
	QLabel* downContentLabel;
	QTimer* timer;
	int m_type;
	
};
