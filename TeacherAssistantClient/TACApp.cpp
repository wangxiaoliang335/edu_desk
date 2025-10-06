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

class MyWidget : public QDialog {
public:
	MyWidget(QWidget* parent = nullptr) : QDialog(parent) {
		setWindowTitle("ģ̬����");
		setModal(true);  // ���� setWindowModality(Qt::ApplicationModal);
	}
};

TACApp::TACApp(int& argc, char** argv) : QApplication(argc, argv)
{
	setDesktopFileName(TAC_APP_NAME);
	setWindowIcon(TAC_APP_ICON);
}
TACApp::~TACApp()
{
}
bool TACApp::AppInit()
{
	setQuitOnLastWindowClosed(false);

	loginWidget = new ModalDialog();
	//loginWidget->setTitleName("ʾ��ģ̬�Ի���");
	loginWidget->setBackgroundColor(QColor(30, 60, 90));
	loginWidget->setBorderColor(Qt::yellow);
	loginWidget->setBorderWidth(4);
	loginWidget->setRadius(12);
	//loginWidget->resize(300, 200);

	//loginWidget->set
	loginWidget->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	loginWidget->setBorderColor(WIDGET_BORDER_COLOR);
	loginWidget->setBorderWidth(WIDGET_BORDER_WIDTH);
	loginWidget->setRadius(30);
	//connect(loginWidget, ModalDialog::pwdLogin,this, )
	int logRes = loginWidget->exec();
	//loginWidget->show();
	// ����ִ�У�ֱ���û��ر�
	if (logRes == QDialog::Accepted) {
		qDebug("�û������ȷ��");
		if (loginWidget->getIsPwdLogin() == true)
		{
			pwdLoginModalDialog = new PwdLoginModalDialog();
			pwdLoginModalDialog->setBackgroundColor(QColor(30, 60, 90));
			pwdLoginModalDialog->setBorderColor(Qt::yellow);
			pwdLoginModalDialog->setBorderWidth(4);
			pwdLoginModalDialog->setRadius(12);
			//loginWidget->resize(300, 200);

			pwdLoginModalDialog->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
			pwdLoginModalDialog->setBorderColor(WIDGET_BORDER_COLOR);
			pwdLoginModalDialog->setBorderWidth(WIDGET_BORDER_WIDTH);
			pwdLoginModalDialog->setRadius(30);
			if (pwdLoginModalDialog->exec() == QDialog::Accepted)
			{

			}
			else
			{
				qDebug("�û��رջ�ȡ��");
				return false;
			}
		}
		else if (loginWidget->getRegisterLogin() == true)
		{
			regDlg = new RegisterDialog();
			regDlg->setBackgroundColor(QColor(30, 60, 90));
			regDlg->setBorderColor(Qt::yellow);
			regDlg->setBorderWidth(4);
			regDlg->setRadius(12);
			//loginWidget->resize(300, 200);

			regDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
			regDlg->setBorderColor(WIDGET_BORDER_COLOR);
			regDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
			regDlg->setRadius(30);
			if (regDlg->exec() == QDialog::Accepted)
			{

			}
			else
			{
				qDebug("�û��رջ�ȡ��");
				return false;
			}
		}
		else if (loginWidget->getResetPwdLogin() == true)
		{
			resetPwdDlg = new ResetPwdDialog();
			resetPwdDlg->setBackgroundColor(QColor(30, 60, 90));
			resetPwdDlg->setBorderColor(Qt::yellow);
			resetPwdDlg->setBorderWidth(4);
			resetPwdDlg->setRadius(12);
			//loginWidget->resize(300, 200);

			resetPwdDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
			resetPwdDlg->setBorderColor(WIDGET_BORDER_COLOR);
			resetPwdDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
			resetPwdDlg->setRadius(30);
			if (resetPwdDlg->exec() == QDialog::Accepted)
			{

			}
			else
			{
				qDebug("�û��رջ�ȡ��");
				return false;
			}
		}
	}
	else if(logRes == QDialog::Rejected) {
		qDebug("�û��رջ�ȡ��");
		return false;
	}

	mainWindow = new TACMainDialog();
	mainWindow->Init();
	mainWindow->show();
	return true;
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