#pragma once

#include <QObject>
#include <QLabel>
#include "TAFloatingWidget.h"
class TACCountDownWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACCountDownWidget(QWidget *parent);
	~TACCountDownWidget();

	void setContent(const QString& text);
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
	void mouseDoubleClickEvent(QMouseEvent* event) override;
signals:
	void doubleClicked();

private:
	QString content;
	QLabel* contentLabel;
};
