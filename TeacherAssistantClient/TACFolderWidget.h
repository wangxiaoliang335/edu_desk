#pragma once

#include "TAFloatingWidget.h"

class TACFolderWidget  : public TAFloatingWidget
{
	Q_OBJECT

public:
	TACFolderWidget(QWidget *parent);
	~TACFolderWidget();
signals:
	void clicked();
protected:
	void initShow() override;
};
