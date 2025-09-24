#pragma once

#include "TABaseDialog.h"
#include <QScrollArea>
#include <QList>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "TAEntity.h"
class TACIMMessageItemWidget : public QWidget
{
	Q_OBJECT
public:
	TACIMMessageItemWidget(TAIMMessage *message,QWidget* parent = nullptr);
	~TACIMMessageItemWidget();

};
class TACIMDialog  : public TABaseDialog
{
	Q_OBJECT

public:
	TACIMDialog(QWidget *parent);
	~TACIMDialog();
	void updateMessage(const QList<TATeacher*>& teacherList,QList<TAIMMessage*>& messageList);
private:
	QHBoxLayout* teacherListLayout;
	QVBoxLayout* messageLayout;
};
