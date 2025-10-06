#pragma once

#include "TAFloatingWidget.h"
#include "SearchDialog.h"
#include "ClassTeacherDialog.h"
class TACAddGroupWidget : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACAddGroupWidget(QWidget *parent);
	~TACAddGroupWidget();
protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void initShow();
private:
	SearchDialog* m_searchDlg = NULL;
	ClassTeacherDialog* m_classTeacherDlg = NULL;
};
