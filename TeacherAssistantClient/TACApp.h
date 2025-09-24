#pragma once
#include <QApplication>
#include <QPointer>
#include <QThread>
#include <functional>
#include "TACMainDialog.h"
typedef std::function<void()> VoidFunc;
class TACApp : public QApplication {
	Q_OBJECT

public:
	TACApp(int& argc, char** argv);
	~TACApp();

	void AppInit();
	QString GetClienId();

public slots:
	void Exec(VoidFunc func);

private:
	bool notify(QObject* receiver, QEvent* e) override;

private:
	QPointer<TACMainDialog> mainWindow;
};
