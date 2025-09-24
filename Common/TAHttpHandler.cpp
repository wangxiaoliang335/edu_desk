#include "TAHttpHandler.h"

TAHttpHandler::TAHttpHandler(QObject* parent)
    : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this,
        &TAHttpHandler::onFinished);

}
void TAHttpHandler::onFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QString responseString = reply->readAll();
        emit success(responseString);
    }
    else {
        emit failed(reply->errorString());
    }
    reply->deleteLater();
}
TAHttpHandler::~TAHttpHandler() {}
void TAHttpHandler::get(const QString& urlStr) {

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(QUrl(urlStr));

    QNetworkReply* reply = manager->get(request);
}

void TAHttpHandler::addHeader(const QString& key, const QString& value)
{
    request.setRawHeader(key.toUtf8(), value.toUtf8());
}

void TAHttpHandler::post(const QString& urlStr, const QMap<QString, QString>& params) {
    request.setUrl(QUrl(urlStr));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonObject;
    for (const QString& name : params.keys()) {
        jsonObject[name] = params.value(name);
    }
    QJsonDocument jsonDoc(jsonObject);
    QByteArray jsonData = jsonDoc.toJson();

    QNetworkReply* reply = manager->post(request, jsonData);
}