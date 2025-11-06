#pragma once

#include "TAFloatingWidget.h"
#include "SearchDialog.h"
#include "ClassTeacherDialog.h"
class TACAddGroupWidget : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACAddGroupWidget(QWidget *parent, TaQTWebSocket* pWs);
	~TACAddGroupWidget();

	void InitData();
	void InitWebSocket();
	QVector<QString> getNoticeMsg();
signals:
	void groupJoined(const QString& groupId); // 加入群组成功信号，转发SearchDialog的信号
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
private:
	SearchDialog* m_searchDlg = NULL;
	ClassTeacherDialog* m_classTeacherDlg = NULL;
};
