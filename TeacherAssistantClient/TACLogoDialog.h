#pragma once

#include "TADialog.h"
#include <QLineEdit>
class TACLogoDialog  : public TADialog
{
	Q_OBJECT

public:
	TACLogoDialog(QWidget *parent);
	~TACLogoDialog();
	QString getSchoolName() const;
	QString getClassName();
	QString getLogoFileName();
	QString getOtherFileName();
private:
	QLineEdit* schoolNameEdit;
	QLineEdit* classNameEdit;
	QString logoFileName;
	QString otherFileName;
};
