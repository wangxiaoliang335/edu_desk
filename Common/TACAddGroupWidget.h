#pragma once
#pragma execution_character_set("utf-8")

#include "TAFloatingWidget.h"
#include "SearchDialog.h"
#include "ClassTeacherDialog.h"
#include "NormalGroupDialog.h"
class TACAddGroupWidget1 : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACAddGroupWidget1(QWidget *parent, TaQTWebSocket* pWs);
	~TACAddGroupWidget1();

	void InitData();
	void InitWebSocket();
	QVector<QString> getNoticeMsg();
signals:
	void groupJoined(const QString& groupId); // 加入群组成功信号，转发SearchDialog的信号
	void groupCreated(const QString& groupId); // 群组创建成功信号，转发ClassTeacherDialog的信号
	void scheduleDialogNeeded(const QString& groupId, const QString& groupName); // 需要创建/初始化ScheduleDialog的信号
	void scheduleDialogRefreshNeeded(const QString& groupId); // 需要刷新ScheduleDialog成员列表的信号
protected:
	//void showEvent(QShowEvent* event) override;
	//void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
private:
	SearchDialog* m_searchDlg = NULL;
	ClassTeacherDialog* m_classTeacherDlg = NULL;
	NormalGroupDialog* m_normalGroupDlg = NULL;
	QPointer<QVBoxLayout> contentLayout;
};
