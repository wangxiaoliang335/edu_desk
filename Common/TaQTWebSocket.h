#pragma once

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <qvector.h>
#include <qdialog.h>

class TaQTWebSocket  : public QObject
{
	Q_OBJECT

public:
	explicit TaQTWebSocket(QObject *parent);
	~TaQTWebSocket();
	static void InitWebSocket(TaQTWebSocket* wsInstance);
	static void regRecvDlg(QDialog* dlg);
	static void sendPrivateMessage(QString msg);
	static void sendBinaryMessage(QByteArray packet);
signals:
	void newMessage(QString msg);

private slots:
	void onConnected();
	void onMessageReceived(const QString& msg);
	void sendBroadcast();
	void sendHeartbeat();
private:
	static QTimer* heartbeatTimer;
	static QWebSocket* socket;
	static QVector<QString> m_NoticeMsg;
	static QVector<QDialog*> m_vecRecvDlg;
};

