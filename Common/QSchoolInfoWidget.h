#pragma once

#include <QWidget>
#include <qlabel.h>
#include <qlineedit.h>
#include "TABaseDialog.h"
#include "TAHttpHandler.h"
class QSchoolInfoWidget  : public QWidget
{
	Q_OBJECT

public:
	QSchoolInfoWidget(QWidget *parent);
	~QSchoolInfoWidget();

	void InitData(UserInfo userInfo);
	QString getSchoolId();
private:
	TAHttpHandler* m_httpHandler = NULL;
	QLabel* errLabel = NULL;  //错误提示信息
	QLineEdit* editCode = NULL;  // 组织代码输入框
	QLineEdit* editSchool = NULL;
	QLineEdit* editAddr = NULL;
	UserInfo m_userInfo;
};
