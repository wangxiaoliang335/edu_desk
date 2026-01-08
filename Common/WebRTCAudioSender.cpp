#include "WebRTCAudioSender.h"

#include <QAudioFormat>
#include <QUrl>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <exception>

// OfferObserver 类已移除，因为 WebRTC 功能已禁用

WebRTCAudioSender::WebRTCAudioSender(QObject* parent)
    : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &WebRTCAudioSender::onSrsApiReply);
    
    qDebug() << "WebRTCAudioSender: 创建实例";
}

WebRTCAudioSender::~WebRTCAudioSender()
{
    disconnectFromServer();
    cleanupWebRTC();
    cleanupAudioInput();
    qDebug() << "WebRTCAudioSender: 销毁实例";
}

bool WebRTCAudioSender::connectToServer(const QString& serverUrl)
{
    // 注意：此功能已禁用。现在语音功能直接使用浏览器实现，不再需要本地 WebRTC 接口。
    // 此方法直接返回 false，不执行任何 WebRTC 相关操作。
    Q_UNUSED(serverUrl);
    qDebug() << "WebRTCAudioSender: 此功能已禁用，语音现在使用浏览器实现";
    return false;
}

void WebRTCAudioSender::disconnectFromServer()
{
    if (!m_isConnected) {
        return;
    }

    qDebug() << "WebRTCAudioSender: 断开连接";

    // 停止音频输入
    cleanupAudioInput();

    // 断开 WebRTC 连接
    cleanupWebRTC();

    m_isConnected = false;
    emit connectionStateChanged(false);
    
    qDebug() << "WebRTCAudioSender: 已断开连接";
}

bool WebRTCAudioSender::initializeWebRTC()
{
    // 注意：此功能已禁用。现在语音功能直接使用浏览器实现，不再需要本地 WebRTC 接口。
    qDebug() << "WebRTCAudioSender: WebRTC 初始化已禁用";
    return false;
    
    // 以下代码已禁用，保留用于参考
    /*
    qDebug() << "WebRTCAudioSender: 初始化 WebRTC";
    
    // 创建线程
    m_networkThread = rtc::Thread::CreateWithSocketServer();
    if (!m_networkThread) {
        qDebug() << "WebRTCAudioSender: 创建网络线程失败";
        return false;
    }
    m_networkThread->SetName("network_thread", nullptr);
    if (!m_networkThread->Start()) {
        qDebug() << "WebRTCAudioSender: 启动网络线程失败";
        return false;
    }
    
    m_workerThread = rtc::Thread::Create();
    if (!m_workerThread) {
        qDebug() << "WebRTCAudioSender: 创建工作线程失败";
        cleanupWebRTC();
        return false;
    }
    m_workerThread->SetName("worker_thread", nullptr);
    if (!m_workerThread->Start()) {
        qDebug() << "WebRTCAudioSender: 启动工作线程失败";
        cleanupWebRTC();
        return false;
    }
    
    m_signalingThread = rtc::Thread::Create();
    if (!m_signalingThread) {
        qDebug() << "WebRTCAudioSender: 创建信令线程失败";
        cleanupWebRTC();
        return false;
    }
    m_signalingThread->SetName("signaling_thread", nullptr);
    if (!m_signalingThread->Start()) {
        qDebug() << "WebRTCAudioSender: 启动信令线程失败";
        cleanupWebRTC();
        return false;
    }
    
    // 创建 PeerConnectionFactory
    if (!createPeerConnectionFactory()) {
        qDebug() << "WebRTCAudioSender: 创建 PeerConnectionFactory 失败";
        cleanupWebRTC();
        return false;
    }
    
    // 创建 PeerConnection
    if (!createPeerConnection()) {
        qDebug() << "WebRTCAudioSender: 创建 PeerConnection 失败";
        cleanupWebRTC();
        return false;
    }
    
    // 创建并添加音频轨道
    if (!createAndAddAudioTrack()) {
        qDebug() << "WebRTCAudioSender: 创建音频轨道失败";
        cleanupWebRTC();
        return false;
    }
    
    return true;
    */
}

void WebRTCAudioSender::cleanupWebRTC()
{
    // WebRTC 功能已禁用，此方法为空实现
    qDebug() << "WebRTCAudioSender: 清理 WebRTC 资源（已禁用）";
}

bool WebRTCAudioSender::createPeerConnectionFactory()
{
    // WebRTC 功能已禁用，此方法为空实现
    qDebug() << "WebRTCAudioSender: 创建 PeerConnectionFactory（已禁用）";
    return false;
}

bool WebRTCAudioSender::createPeerConnection()
{
    // WebRTC 功能已禁用，此方法为空实现
    qDebug() << "WebRTCAudioSender: 创建 PeerConnection（已禁用）";
    return false;
}

bool WebRTCAudioSender::createAndAddAudioTrack()
{
    // WebRTC 功能已禁用，此方法为空实现
    qDebug() << "WebRTCAudioSender: 创建音频轨道（已禁用）";
    return false;
}

void WebRTCAudioSender::createOfferAndConnect()
{
    // WebRTC 功能已禁用，此方法为空实现
    qDebug() << "WebRTCAudioSender: 创建 Offer（已禁用）";
}

void WebRTCAudioSender::exchangeSdpWithSrs(const QString& offer)
{
    qDebug() << "WebRTCAudioSender: 通过 SRS API 交换 SDP";
    
    if (offer.isEmpty()) {
        qDebug() << "WebRTCAudioSender: Offer SDP 为空";
        emit errorOccurred("Offer SDP 为空");
        return;
    }
    
    if (!m_networkManager) {
        qDebug() << "WebRTCAudioSender: 网络管理器为空";
        emit errorOccurred("网络管理器为空");
        return;
    }
    
    // SRS WebRTC API: POST /rtc/v1/publish/
    QString apiUrl = QString("http://%1:%2/rtc/v1/publish/").arg(m_srsHost).arg(m_srsPort);
    
    QJsonObject requestJson;
    requestJson["api"] = "publish";
    requestJson["streamurl"] = QString("webrtc://%1:%2/live/%3").arg(m_srsHost).arg(m_srsPort).arg(m_streamName);
    requestJson["sdp"] = offer;
    
    QJsonDocument doc(requestJson);
    QByteArray data = doc.toJson();
    
    QUrl url(apiUrl);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = m_networkManager->post(request, data);
    qDebug() << "WebRTCAudioSender: 发送 SDP Offer 到" << apiUrl;
}

bool WebRTCAudioSender::parseServerUrl(const QString& url, QString& host, int& port, QString& streamName)
{
    // 解析格式: webrtc://host:port/live/streamName
    QUrl qurl(url);
    
    if (!qurl.isValid() || qurl.scheme() != "webrtc") {
        qDebug() << "WebRTCAudioSender: 无效的 URL 格式:" << url;
        return false;
    }
    
    host = qurl.host();
    port = qurl.port();
    if (port == -1) {
        port = 1985;  // 默认端口
    }
    
    // 提取流名称（从路径中）
    QString path = qurl.path();
    if (path.startsWith("/live/")) {
        streamName = path.mid(6);  // 去掉 "/live/" 前缀
    } else {
        streamName = path.mid(1);  // 去掉开头的 "/"
    }
    
    if (streamName.isEmpty()) {
        streamName = "stream1";  // 默认流名称
    }
    
    return true;
}

void WebRTCAudioSender::initAudioInput()
{
    qDebug() << "WebRTCAudioSender: 初始化音频输入";

    // 配置音频格式
    QAudioFormat format;
    format.setSampleRate(48000);  // WebRTC 通常使用 48kHz
    format.setChannelCount(1);    // 单声道
    format.setSampleSize(16);     // 16 位
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // 检查格式是否支持
    QAudioDeviceInfo deviceInfo = QAudioDeviceInfo::defaultInputDevice();
    if (!deviceInfo.isFormatSupported(format)) {
        qDebug() << "WebRTCAudioSender: 音频格式不支持，使用最接近的格式";
        format = deviceInfo.nearestFormat(format);
    }

    // 创建音频输入
    m_audioInput = new QAudioInput(deviceInfo, format, this);
    m_audioInputDevice = m_audioInput->start();

    if (!m_audioInputDevice) {
        qDebug() << "WebRTCAudioSender: 无法启动音频输入设备";
        delete m_audioInput;
        m_audioInput = nullptr;
        return;
    }

    // 连接音频数据就绪信号
    connect(m_audioInputDevice, &QIODevice::readyRead, this, &WebRTCAudioSender::onAudioDataReady);

    qDebug() << "WebRTCAudioSender: 音频输入初始化成功"
             << "采样率:" << format.sampleRate()
             << "声道数:" << format.channelCount()
             << "采样大小:" << format.sampleSize();
}

void WebRTCAudioSender::cleanupAudioInput()
{
    if (m_audioInput) {
        if (m_audioInputDevice) {
            m_audioInput->stop();
            m_audioInputDevice = nullptr;
        }
        delete m_audioInput;
        m_audioInput = nullptr;
        qDebug() << "WebRTCAudioSender: 音频输入已清理";
    }
}

void WebRTCAudioSender::onAudioDataReady()
{
    if (!m_isConnected || !m_audioInputDevice) {
        return;
    }

    // 读取音频数据
    QByteArray audioData = m_audioInputDevice->readAll();
    if (audioData.isEmpty()) {
        return;
    }

    // 发送音频数据到 WebRTC
    sendAudioData(audioData);
}

void WebRTCAudioSender::sendAudioData(const QByteArray& audioData)
{
    // WebRTC 功能已禁用，此方法为空实现
    Q_UNUSED(audioData);
    qDebug() << "WebRTCAudioSender: 发送音频数据（已禁用）";
}

void WebRTCAudioSender::onSrsApiReply(QNetworkReply* reply)
{
    if (!reply) {
        qDebug() << "WebRTCAudioSender: 网络回复为空";
        emit errorOccurred("网络回复为空");
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "WebRTCAudioSender: SRS API 请求失败:" << reply->errorString();
        emit errorOccurred(QString("SRS API 请求失败: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "WebRTCAudioSender: 解析 SRS API 响应失败:" << error.errorString();
        emit errorOccurred("解析 SRS API 响应失败");
        reply->deleteLater();
        return;
    }
    
    QJsonObject response = doc.object();
    
    if (response.contains("code") && response["code"].toInt() != 0) {
        QString errorMsg = response.value("code").toString();
        qDebug() << "WebRTCAudioSender: SRS API 返回错误:" << errorMsg;
        emit errorOccurred(QString("SRS API 错误: %1").arg(errorMsg));
        reply->deleteLater();
        return;
    }
    
    // WebRTC 功能已禁用，此方法不再处理 SDP Answer
    qDebug() << "WebRTCAudioSender: SRS API 响应已收到（WebRTC 功能已禁用，不处理）";
    reply->deleteLater();
}

// PeerConnectionObserver 接口实现已移除（WebRTC 功能已禁用）

// CreateSessionDescriptionObserver 接口实现已移除（WebRTC 功能已禁用）

// 无需恢复 Qt 宏（此文件中未修改）
