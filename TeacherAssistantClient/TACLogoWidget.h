#pragma once

#include "TAFloatingWidget.h"
#include <QPainter>
#include <QPixmap>
#include <QPaintEvent>
#include <QLabel>
#include <QMouseEvent>
class TACLogoWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACLogoWidget(QWidget *parent);
	~TACLogoWidget();
	void updateLogo(const QString& fileName);
protected:
	void paintEvent(QPaintEvent* event) override;
	void initShow() override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
signals:
	void doubleClicked();

private:
	QString m_fileName;

};
class TACSchoolLabelWidget : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACSchoolLabelWidget(QWidget* parent);
	~TACSchoolLabelWidget();
	void setContent(const QString& text);
protected:
	void initShow() override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
signals:
	void doubleClicked();
private:
	QLabel* label;
};
class TACClassLabelWidget : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACClassLabelWidget(QWidget* parent);
	~TACClassLabelWidget();
	void setContent(const QString& text);
protected:
	void initShow() override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
signals:
	void doubleClicked();

private:
	QLabel* label;
};