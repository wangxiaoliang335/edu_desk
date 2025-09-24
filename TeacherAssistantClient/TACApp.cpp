#include "TACApp.h"
#include <QIcon>
#include "common.h"
inline TACApp* App()
{
	return static_cast<TACApp*>(qApp);
}
static inline Qt::ConnectionType WaitConnection()
{
	return QThread::currentThread() == qApp->thread()
		? Qt::DirectConnection
		: Qt::BlockingQueuedConnection;
}
static void ui_task_handler(void* param, bool wait)
{
	auto doTask = [=]() {};
	QMetaObject::invokeMethod(App(), "Exec",
		wait ? WaitConnection() : Qt::AutoConnection,
		Q_ARG(VoidFunc, doTask));
}
TACApp::TACApp(int& argc, char** argv) : QApplication(argc, argv)
{
	setDesktopFileName(TAC_APP_NAME);
	setWindowIcon(TAC_APP_ICON);
}
TACApp::~TACApp()
{
}
void TACApp::AppInit()
{
	setQuitOnLastWindowClosed(false);
	mainWindow = new TACMainDialog();
	mainWindow->Init();
	mainWindow->show();
}
QString TACApp::GetClienId()
{
	return QString();
}
void TACApp::Exec(VoidFunc func)
{
	func();
}
bool TACApp::notify(QObject* receiver, QEvent* e)
{
	return QApplication::notify(receiver, e);
}