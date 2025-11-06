#include <QGuiApplication>
#include <QApplication>
#include <QWindow>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QMutexLocker>
#include <QDir>
#include <QTime>
#include <QSslSocket>
#include <QSslConfiguration>
#include "common.h"
#include "TACApp.h"
#ifdef _WIN32
#include <windows.h>
#include <filesystem>
#include <comdef.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef _MSC_VER
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

QFile* logFile = nullptr;
QMutex logMutex;
QString getLogFileName() {
	return QString("tac-%1.log").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));
}
void customLogHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	QMutexLocker locker(&logMutex);
	QDir dir;
	if (!dir.exists("./logs")) {
		dir.mkdir("./logs");
	}
	QString fileName = "logs/" + getLogFileName();
	if (logFile == nullptr || logFile->fileName() != fileName) {
		if (logFile != nullptr) {
			logFile->close();
			delete logFile;
			logFile = nullptr;
		}
		logFile = new QFile(fileName);
		if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
			fprintf(stderr, "Unable to open log file %s for writing: %s\n",
				qPrintable(fileName), qPrintable(logFile->errorString()));
			delete logFile;
			logFile = nullptr;
			return;
		}
	}
	QTextStream ts(logFile);
	ts << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " ";

	switch (type) {
	case QtDebugMsg:
		ts << "DEBUG: ";
		break;
	case QtInfoMsg:
		ts << "INFO: ";
		break;
	case QtWarningMsg:
		ts << "WARNING: ";
		break;
	case QtCriticalMsg:
		ts << "CRITICAL: ";
		break;
	case QtFatalMsg:
		ts << "FATAL: ";
		break;
	}
	ts << msg << Qt::endl;
	ts.flush();
}
static void load_debug_privilege(void)
{
	const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
	TOKEN_PRIVILEGES tp;
	HANDLE token;
	LUID val;

	if (!OpenProcessToken(GetCurrentProcess(), flags, &token)) {
		return;
	}

	if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL,
			NULL);
	}

	if (!!LookupPrivilegeValue(NULL, SE_INC_BASE_PRIORITY_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL,
			NULL)) {
			qWarning()<< "Could not set privilege to increase GPU priority";
		}
	}

	CloseHandle(token);
}

void debugOutputHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	QByteArray localMsg = msg.toLocal8Bit();
	const char* prefix = "";
	switch (type) {
	case QtDebugMsg:    prefix = "[Debug] "; break;
	case QtInfoMsg:     prefix = "[Info] "; break;
	case QtWarningMsg:  prefix = "[Warning] "; break;
	case QtCriticalMsg: prefix = "[Critical] "; break;
	case QtFatalMsg:    prefix = "[Fatal] "; break;
	}

#ifdef Q_OS_WIN
	// ���͵� Visual Studio ���������
	OutputDebugStringA((QString(prefix) + QString(localMsg)).toLocal8Bit().data());
#else
	// Mac/Linuxֱ�ӷ�����׼�����Ҳ�ᱻ Qt Creator ����
	fprintf(stderr, "%s%s
		", prefix, localMsg.constData());
#endif
}

int main(int argc, char* argv[])
{
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	// ��װ����ض���
	qInstallMessageHandler(debugOutputHandler);

#ifdef _WIN32
	SetErrorMode(SEM_FAILCRITICALERRORS);
	load_debug_privilege();
	
	const HMODULE hRtwq = LoadLibrary(L"RTWorkQ.dll");
	if (hRtwq) {
		typedef HRESULT(STDAPICALLTYPE* PFN_RtwqStartup)();
		PFN_RtwqStartup func =
			(PFN_RtwqStartup)GetProcAddress(hRtwq, "RtwqStartup");
		func();
	}
#endif

#ifdef _WIN32
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
		Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

	QCoreApplication::addLibraryPath(".");

	qputenv("QT_NO_SUBTRACTOPAQUESIBLINGS", "1");
	qInstallMessageHandler(customLogHandler);
	qInfo() << "teacher assistant start ,current version is  " << TAC_VERSION;
	TACApp program(argc, argv);
	QFile qssFile(":/res/css/main.css");
	if (qssFile.open(QFile::ReadOnly))
	{
		QString style = QLatin1String(qssFile.readAll());
		program.setStyleSheet(style);
		qssFile.close();
	}

	if (true == program.AppInit())
	{
		program.exec();
	}
}