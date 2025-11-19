#pragma once

#include "TAFloatingWidget.h"

class TACTrayWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACTrayWidget(QWidget *parent);
	~TACTrayWidget();
	void updateAdminButtonState(); // 更新管理员按钮状态
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
	QPushButton* m_fileManagerButton; // 保存按钮指针
signals:
	void navType(bool checked = false);
	void navChoolInfo(bool checked = false);
};
