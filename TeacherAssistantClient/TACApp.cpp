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
	bool bLoginSucceeded = false;
	QString qPhone;
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

	regDlg = new RegisterDialog();
	regDlg->setBackgroundColor(QColor(30, 60, 90));
	regDlg->setBorderColor(Qt::yellow);
	regDlg->setBorderWidth(4);
	regDlg->setRadius(12);

	regDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	regDlg->setBorderColor(WIDGET_BORDER_COLOR);
	regDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
	regDlg->setRadius(30);

	resetPwdDlg = new ResetPwdDialog();
	resetPwdDlg->setBackgroundColor(QColor(30, 60, 90));
	resetPwdDlg->setBorderColor(Qt::yellow);
	resetPwdDlg->setBorderWidth(4);
	resetPwdDlg->setRadius(12);

	resetPwdDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
	resetPwdDlg->setBorderColor(WIDGET_BORDER_COLOR);
	resetPwdDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
	resetPwdDlg->setRadius(30);

	connect(this, &TACApp::pwdLogin, this,[&]() {
		if (pwdLoginModalDialog->exec() == QDialog::Accepted)
		{
			if (pwdLoginModalDialog->getVerificationCodeLogin())
			{
				pwdLoginModalDialog->InitData();
				int logRes = loginWidget->exec();
				if (logRes == QDialog::Accepted)
				{
					if (loginWidget->getIsPwdLogin() == true)
					{
						loginWidget->InitData();
						emit pwdLogin();
					}
					else if (loginWidget->getRegisterLogin() == true)
					{
						loginWidget->InitData();
						emit regLogin();
					}
					else if (loginWidget->getResetPwdLogin() == true)
					{
						loginWidget->InitData();
						emit resetPwdLogin();
					}
					else
					{
						bLoginSucceeded = true;
						qPhone = loginWidget->phoneNumber();
					}
				}
			}
			else if (pwdLoginModalDialog->getRegisterLogin() == true)
			{
				pwdLoginModalDialog->InitData();
				emit regLogin();
			}
			else if (pwdLoginModalDialog->getResetPwdLogin() == true)
			{
				pwdLoginModalDialog->InitData();
				emit resetPwdLogin();
			}
			else
			{
				bLoginSucceeded = true;
				qPhone = pwdLoginModalDialog->phoneNumber();
			}
			return true;
		}
		else
		{
			qDebug("�û��رջ�ȡ��");
			bLoginSucceeded = false;
			return false;
		}
	});

	connect(this, &TACApp::regLogin, this, [&]() {
		if (regDlg->exec() == QDialog::Accepted)
		{
			//regDlg->InitData();
			if (regDlg->getIsPwdLogin())
			{
				regDlg->InitData();
				emit pwdLogin();
			}
			else if (regDlg->getResetPwdLogin() == true)
			{
				regDlg->InitData();
				emit resetPwdLogin();
			}
			else
			{
				bLoginSucceeded = true;
				qPhone = regDlg->phoneNumber();
			}
			return true;
		}
		else
		{
			qDebug("�û��رջ�ȡ��");
			bLoginSucceeded = false;
			return false;
		}
	});

	connect(this, &TACApp::resetPwdLogin, this, [&]() {
		if (resetPwdDlg->exec() == QDialog::Accepted)
		{
			if (resetPwdDlg->getIsPwdLogin())
			{
				resetPwdDlg->InitData();
				emit pwdLogin();
			}
			else
			{
				bLoginSucceeded = true;
				qPhone = resetPwdDlg->phoneNumber();
			}
			return true;
		}
		else
		{
			qDebug("�û��رջ�ȡ��");
			bLoginSucceeded = false;
			return false;
		}
	});
	
	int logRes = loginWidget->exec();
	
	// ����ִ�У�ֱ���û��ر�
	if (logRes == QDialog::Accepted) {
		qDebug("�û������ȷ��");
		if (loginWidget->getIsPwdLogin() == true)
		{
			loginWidget->InitData();
			emit pwdLogin();
		}
		else if (loginWidget->getRegisterLogin() == true)
		{
			loginWidget->InitData();
			emit regLogin();
		}
		else if (loginWidget->getResetPwdLogin() == true)
		{
			loginWidget->InitData();
			emit resetPwdLogin();
		}
		else
		{
			bLoginSucceeded = true;
			qPhone = loginWidget->phoneNumber();
		}
	}
	else if(logRes == QDialog::Rejected) {
		qDebug("�û��رջ�ȡ��");
		return false;
	}

	if (bLoginSucceeded)
	{
		mainWindow = new TACMainDialog();
		mainWindow->Init(qPhone);
		mainWindow->show();
		return true;
	}
	else
	{
		return false;
	}
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