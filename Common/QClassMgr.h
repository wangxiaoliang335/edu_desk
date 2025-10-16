#pragma once

#include <QWidget>
#include <qtablewidget.h>
#include "TAHttpHandler.h"
class QClassMgr  : public QWidget
{
	Q_OBJECT

public:
	QClassMgr(QWidget *parent);
	~QClassMgr();

	void setSchoolId(QString schoolId);
	void initData();
private:
	void fillRow(int row);
	QString generateClassIdAuto(const QString& xueduan, const QString& nianji);

public:
	QTableWidget* table_ = NULL;
	QString m_schoolId;
	TAHttpHandler* m_httpHandler = NULL;
};

