#pragma once
#include <QApplication>
#include <QPointer>
#include <QThread>
#include <functional>
#include "TACMainDialog.h"
#include "ModalDialog.h"
#include "PwdLoginModalDialog.h"
#include "RegisterDialog.h"
#include "ResetPwdDialog.h"

typedef std::function<void()> VoidFunc;
class TACApp : public QApplication {
	Q_OBJECT

public:
	TACApp(int& argc, char** argv);
	~TACApp();

	bool AppInit();
	QString GetClienId();

public slots:
	void Exec(VoidFunc func);

private:
	bool notify(QObject* receiver, QEvent* e) override;

private:
	QPointer<TACMainDialog> mainWindow;
	QPointer<ModalDialog> loginWidget;
	QPointer<PwdLoginModalDialog> pwdLoginModalDialog;
	QPointer<RegisterDialog> regDlg;
	QPointer<ResetPwdDialog> resetPwdDlg;
};
