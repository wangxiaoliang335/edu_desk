#pragma once

#include <QObject>
#include <QNetworkAccessManager>  
#include <QNetworkRequest>  
#include <QNetworkReply>  
#include <QJsonDocument>  
#include <QJsonObject>  
#include <QJsonArray> 
#include <QPointer>
class TAHttpHandler : public QObject
{
	Q_OBJECT

public:
	TAHttpHandler(QObject* parent = nullptr);
	~TAHttpHandler();
	void get(const QString& urlStr);
	void addHeader(const QString& key, const QString& value);
	void post(const QString& urlStr, const QMap<QString, QString>& params);

signals:
	void success(const QString content);
	void failed(const QString content);

private:
	QNetworkAccessManager* manager;
	QNetworkRequest request;
	void onFinished(QNetworkReply* reply);

};