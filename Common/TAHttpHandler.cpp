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
        QByteArray responseString = reply->readAll();
        QString responseText = QString::fromUtf8(responseString);
        qDebug() << "纯文本:" << responseText;
        // 解析 JSON
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString);
        if (jsonDoc.isObject()) {
            QJsonObject obj = jsonDoc.object();
            if (obj["data"].isObject())
            {
                QJsonObject oTmp = obj["data"].toObject();
                QString strTmp = oTmp["message"].toString();
                qDebug() << "status:" << oTmp["code"].toString();
                qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
            }
        }
        emit success(responseString);
    }
    else {
        //emit failed(reply->errorString());
        emit failed(reply->readAll());
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