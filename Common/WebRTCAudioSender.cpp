// 在包含 WebRTC 头文件之前定义 NOMINMAX，避免 Windows.h 的 max/min 宏冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

// WebRTC 头文件优先
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/jsep.h"
#include "api/rtc_error.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "rtc_base/helpers.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"

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

class OfferObserver : public webrtc::CreateSessionDescriptionObserver {
public:
    explicit OfferObserver(WebRTCAudioSender* sender) : m_sender(sender) {}

    void OnSuccess(webrtc::SessionDescriptionInterface* desc) override
    {
        if (m_sender) {
            m_sender->onCreateOfferSuccess(desc);
        }
    }

    void OnFailure(webrtc::RTCError error) override
    {
        if (m_sender) {
            m_sender->onCreateOfferFailure(error);
        }
    }

private:
    WebRTCAudioSender* m_sender = nullptr;
};

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
    if (serverUrl.isEmpty()) {
        qDebug() << "WebRTCAudioSender: 服务器 URL 为空";
        emit errorOccurred("服务器 URL 为空");
        return false;
    }

    if (m_isConnected) {
        qDebug() << "WebRTCAudioSender: 已经连接，先断开";
        disconnectFromServer();
    }

    m_serverUrl = serverUrl;
    
    // 解析服务器 URL
    if (!parseServerUrl(serverUrl, m_srsHost, m_srsPort, m_streamName)) {
        qDebug() << "WebRTCAudioSender: 解析服务器 URL 失败" << serverUrl;
        emit errorOccurred("解析服务器 URL 失败");
        return false;
    }
    
    qDebug() << "WebRTCAudioSender: 开始连接到服务器" << serverUrl
             << "Host:" << m_srsHost << "Port:" << m_srsPort << "Stream:" << m_streamName;

    // 初始化 WebRTC
    if (!initializeWebRTC()) {
        qDebug() << "WebRTCAudioSender: WebRTC 初始化失败";
        emit errorOccurred("WebRTC 初始化失败");
        return false;
    }

    // 检查 PeerConnection 是否创建成功
    if (!m_peerConnection) {
        qDebug() << "WebRTCAudioSender: PeerConnection 为空，初始化失败";
        cleanupWebRTC();
        emit errorOccurred("PeerConnection 创建失败");
        return false;
    }

    // 初始化音频输入
    initAudioInput();
    if (!m_audioInput) {
        qDebug() << "WebRTCAudioSender: 音频输入初始化失败";
        cleanupWebRTC();
        emit errorOccurred("音频输入初始化失败");
        return false;
    }

    // 创建 Offer 并连接
    createOfferAndConnect();
    
    return true;
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
}

void WebRTCAudioSender::cleanupWebRTC()
{
    qDebug() << "WebRTCAudioSender: 清理 WebRTC 资源";
    
    // 关闭 PeerConnection
    if (m_peerConnection) {
        m_peerConnection->Close();
        m_peerConnection = nullptr;
    }
    
    // 释放音频轨道
    m_audioTrack = nullptr;
    m_audioSource = nullptr;
    
    // 释放 PeerConnectionFactory
    m_peerConnectionFactory = nullptr;
    
    // 停止并释放线程
    if (m_networkThread) {
        m_networkThread->Stop();
        m_networkThread = nullptr;
    }
    if (m_workerThread) {
        m_workerThread->Stop();
        m_workerThread = nullptr;
    }
    if (m_signalingThread) {
        m_signalingThread->Stop();
        m_signalingThread = nullptr;
    }
}

bool WebRTCAudioSender::createPeerConnectionFactory()
{
    qDebug() << "WebRTCAudioSender: 创建 PeerConnectionFactory";
    
    // 检查线程是否有效
    if (!m_networkThread || !m_workerThread || !m_signalingThread) {
        qDebug() << "WebRTCAudioSender: 线程未初始化";
        return false;
    }
    
    try {
        // 初始化 SSL 适配器（WebRTC 需要，只初始化一次）
        static bool sslInitialized = false;
        if (!sslInitialized) {
            rtc::InitializeSSL();
            sslInitialized = true;
            qDebug() << "WebRTCAudioSender: SSL 已初始化";
        }
        
        qDebug() << "WebRTCAudioSender: 调用 CreatePeerConnectionFactory";
        m_peerConnectionFactory = webrtc::CreatePeerConnectionFactory(
            m_networkThread.get(),
            m_workerThread.get(),
            m_signalingThread.get(),
            nullptr,  // AudioDeviceModule (使用默认)
            webrtc::CreateBuiltinAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            nullptr,  // VideoEncoderFactory
            nullptr,  // VideoDecoderFactory
            nullptr,  // AudioMixer
            nullptr    // AudioProcessing
        );
        
        if (!m_peerConnectionFactory) {
            qDebug() << "WebRTCAudioSender: 创建 PeerConnectionFactory 失败";
            return false;
        }
        
        qDebug() << "WebRTCAudioSender: PeerConnectionFactory 创建成功，地址:" 
                 << static_cast<void*>(m_peerConnectionFactory.get());
        return true;
    } catch (const std::exception& e) {
        qDebug() << "WebRTCAudioSender: 创建 PeerConnectionFactory 时发生异常:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "WebRTCAudioSender: 创建 PeerConnectionFactory 时发生未知异常";
        return false;
    }
}

bool WebRTCAudioSender::createPeerConnection()
{
    qDebug() << "WebRTCAudioSender: 创建 PeerConnection";
    
    // 检查 PeerConnectionFactory 是否有效
    if (!m_peerConnectionFactory) {
        qDebug() << "WebRTCAudioSender: PeerConnectionFactory 为空，无法创建 PeerConnection";
        return false;
    }
    
    // 检查线程是否有效（线程启动失败已在 initializeWebRTC 中处理）
    if (!m_networkThread || !m_workerThread || !m_signalingThread) {
        qDebug() << "WebRTCAudioSender: 线程未初始化";
        return false;
    }
    
    try {
        qDebug() << "WebRTCAudioSender: 开始创建配置";
        webrtc::PeerConnectionInterface::RTCConfiguration config;
        config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
        
        // 添加 STUN 服务器（SRS 通常不需要，但可以添加）
        webrtc::PeerConnectionInterface::IceServer stunServer;
        stunServer.uri = "stun:stun.l.google.com:19302";
        config.servers.push_back(stunServer);
        
        qDebug() << "WebRTCAudioSender: 配置创建完成，STUN 服务器数量:" << config.servers.size();
        
        qDebug() << "WebRTCAudioSender: 创建 PeerConnectionDependencies";
        // 创建 PeerConnection
        webrtc::PeerConnectionDependencies dependencies(this);
        
        // 检查 observer 是否被正确设置
        if (!dependencies.observer) {
            qDebug() << "WebRTCAudioSender: PeerConnectionObserver 为空";
            return false;
        }
        
        qDebug() << "WebRTCAudioSender: 调用 CreatePeerConnectionOrError，PeerConnectionFactory 地址:" 
                 << static_cast<void*>(m_peerConnectionFactory.get());
        
        qDebug() << "WebRTCAudioSender: 开始调用 CreatePeerConnectionOrError";
        qDebug() << "WebRTCAudioSender: dependencies.observer 地址:" 
                 << static_cast<void*>(dependencies.observer);
        
        // 先保存 observer 指针，确保在移动后仍然有效
        webrtc::PeerConnectionObserver* observerPtr = dependencies.observer;
        
        // 直接调用并立即提取值，避免中间变量的问题
        // 注意：不要移动 dependencies，直接传递，让函数内部处理
        webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::PeerConnectionInterface>> result;
        
        try {
            result = m_peerConnectionFactory->CreatePeerConnectionOrError(
                config, std::move(dependencies));
            qDebug() << "WebRTCAudioSender: CreatePeerConnectionOrError 调用完成";
        } catch (const std::exception& e) {
            qDebug() << "WebRTCAudioSender: CreatePeerConnectionOrError 抛出异常:" << e.what();
            return false;
        } catch (...) {
            qDebug() << "WebRTCAudioSender: CreatePeerConnectionOrError 抛出未知异常";
            return false;
        }
        
        qDebug() << "WebRTCAudioSender: CreatePeerConnectionOrError 调用完成，ok()=" << result.ok();
        
        // 检查结果
        if (!result.ok()) {
            qDebug() << "WebRTCAudioSender: 创建 PeerConnection 失败，错误类型:" 
                     << static_cast<int>(result.error().type())
                     << "，错误消息:" << result.error().message();
            return false;
        }
        
        qDebug() << "WebRTCAudioSender: 开始获取 PeerConnection 值";
        // 使用 value() 获取引用并立即复制，避免移动语义的问题
        // 这样 scoped_refptr 会正确增加引用计数
        m_peerConnection = result.value();
        
        qDebug() << "WebRTCAudioSender: 获取值完成，PeerConnection 地址:" 
                 << static_cast<void*>(m_peerConnection.get());
        
        // 现在可以安全地让 result 析构，因为我们已经复制了引用
        // result 析构时会减少引用计数，但对象不会被销毁，因为我们持有引用
        
        if (!m_peerConnection) {
            qDebug() << "WebRTCAudioSender: PeerConnection 创建后为空";
            return false;
        }
        
        qDebug() << "WebRTCAudioSender: PeerConnection 创建成功，地址:" 
                 << static_cast<void*>(m_peerConnection.get());
        return true;
    } catch (const std::exception& e) {
        qDebug() << "WebRTCAudioSender: 创建 PeerConnection 时发生异常:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "WebRTCAudioSender: 创建 PeerConnection 时发生未知异常";
        return false;
    }
}

bool WebRTCAudioSender::createAndAddAudioTrack()
{
    qDebug() << "WebRTCAudioSender: 创建音频轨道";
    
    // 创建音频源
    cricket::AudioOptions options;
    m_audioSource = m_peerConnectionFactory->CreateAudioSource(options);
    
    if (!m_audioSource) {
        qDebug() << "WebRTCAudioSender: 创建音频源失败";
        return false;
    }
    
    // 创建音频轨道
    std::string trackId = rtc::CreateRandomString(16);
    m_audioTrack = m_peerConnectionFactory->CreateAudioTrack(trackId, m_audioSource.get());
    
    if (!m_audioTrack) {
        qDebug() << "WebRTCAudioSender: 创建音频轨道失败";
        return false;
    }
    
    // 添加轨道到 PeerConnection
    auto result = m_peerConnection->AddTrack(m_audioTrack, {trackId});
    
    if (!result.ok()) {
        qDebug() << "WebRTCAudioSender: 添加音频轨道失败:" << result.error().message();
        return false;
    }
    
    qDebug() << "WebRTCAudioSender: 音频轨道创建并添加成功";
    return true;
}

void WebRTCAudioSender::createOfferAndConnect()
{
    qDebug() << "WebRTCAudioSender: 创建 Offer";
    
    if (!m_peerConnection) {
        qDebug() << "WebRTCAudioSender: PeerConnection 为空，无法创建 Offer";
        emit errorOccurred("PeerConnection 为空，无法创建 Offer");
        return;
    }
    
    // 创建 Offer
    rtc::scoped_refptr<webrtc::CreateSessionDescriptionObserver> observer =
        new rtc::RefCountedObject<OfferObserver>(this);
    m_peerConnection->CreateOffer(observer,
        webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
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
    if (!m_isConnected || !m_audioTrack) {
        return;
    }

    // 注意：WebRTC 的音频轨道会自动从 AudioDeviceModule 获取音频数据
    // 这里我们使用 Qt 的 QAudioInput 来采集音频，但需要将其转换为 WebRTC 的格式
    // 实际上，更好的方式是直接使用 WebRTC 的 AudioDeviceModule
    // 这里暂时保留接口，实际音频数据会通过 WebRTC 的音频设备模块自动处理
    
    // TODO: 如果需要自定义音频源，可以使用 webrtc::AudioSourceInterface
    // 并实现 PushAudioData 方法
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
    
    // 获取 Answer SDP
    QString answer = response.value("sdp").toString();
    if (answer.isEmpty()) {
        qDebug() << "WebRTCAudioSender: SRS API 响应中没有 SDP Answer";
        emit errorOccurred("SRS API 响应中没有 SDP Answer");
        reply->deleteLater();
        return;
    }
    
    if (!m_peerConnection) {
        qDebug() << "WebRTCAudioSender: PeerConnection 为空，无法设置远程描述";
        emit errorOccurred("PeerConnection 为空");
        reply->deleteLater();
        return;
    }
    
    qDebug() << "WebRTCAudioSender: 收到 SRS Answer，设置远程描述";
    
    // 设置远程描述（Answer）
    webrtc::SdpParseError parseError;
    std::unique_ptr<webrtc::SessionDescriptionInterface> sessionDescription(
        webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, answer.toStdString(), &parseError));
    
    if (!sessionDescription) {
        qDebug() << "WebRTCAudioSender: 解析 Answer SDP 失败:" << parseError.description.c_str();
        emit errorOccurred("解析 Answer SDP 失败");
        reply->deleteLater();
        return;
    }
    
    // 创建 SetRemoteDescriptionObserver
    class SetRemoteDescObserver : public webrtc::SetRemoteDescriptionObserverInterface {
    public:
        void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override {
            if (!error.ok()) {
                qDebug() << "WebRTCAudioSender: 设置远程描述失败:" << error.message();
            } else {
                qDebug() << "WebRTCAudioSender: 设置远程描述成功";
            }
        }
    };
    
    rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface> remoteObserver =
        new rtc::RefCountedObject<SetRemoteDescObserver>();
    
    m_peerConnection->SetRemoteDescription(
        std::move(sessionDescription),
        remoteObserver);
    
    reply->deleteLater();
}

// PeerConnectionObserver 接口实现
void WebRTCAudioSender::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
    qDebug() << "WebRTCAudioSender: 信令状态改变:" << new_state;
}

void WebRTCAudioSender::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
    qDebug() << "WebRTCAudioSender: ICE 连接状态改变:" << new_state;
    
    if (new_state == webrtc::PeerConnectionInterface::kIceConnectionConnected ||
        new_state == webrtc::PeerConnectionInterface::kIceConnectionCompleted) {
        if (!m_isConnected) {
            m_isConnected = true;
            emit connectionStateChanged(true);
            qDebug() << "WebRTCAudioSender: WebRTC 连接已建立";
        }
    } else if (new_state == webrtc::PeerConnectionInterface::kIceConnectionDisconnected ||
               new_state == webrtc::PeerConnectionInterface::kIceConnectionFailed ||
               new_state == webrtc::PeerConnectionInterface::kIceConnectionClosed) {
        if (m_isConnected) {
            m_isConnected = false;
            emit connectionStateChanged(false);
            qDebug() << "WebRTCAudioSender: WebRTC 连接已断开";
        }
    }
}

void WebRTCAudioSender::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
    qDebug() << "WebRTCAudioSender: ICE 收集状态改变:" << new_state;
}

void WebRTCAudioSender::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    qDebug() << "WebRTCAudioSender: 收到 ICE 候选";
    // 注意：对于 SRS，ICE 候选通常包含在 SDP 中，不需要单独处理
}

void WebRTCAudioSender::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{
    qDebug() << "WebRTCAudioSender: 收到数据通道";
}

void WebRTCAudioSender::OnRenegotiationNeeded()
{
    qDebug() << "WebRTCAudioSender: 需要重新协商";
}

// CreateSessionDescriptionObserver 接口实现
void WebRTCAudioSender::onCreateOfferSuccess(webrtc::SessionDescriptionInterface* desc)
{
    qDebug() << "WebRTCAudioSender: 创建 Offer 成功";
    
    if (!desc) {
        qDebug() << "WebRTCAudioSender: Offer 描述为空";
        emit errorOccurred("Offer 描述为空");
        return;
    }
    
    if (!m_peerConnection) {
        qDebug() << "WebRTCAudioSender: PeerConnection 为空，无法设置本地描述";
        emit errorOccurred("PeerConnection 为空");
        return;
    }
    
    // 创建 SetLocalDescriptionObserver
    class SetLocalDescObserver : public webrtc::SetLocalDescriptionObserverInterface {
    public:
        void OnSetLocalDescriptionComplete(webrtc::RTCError error) override {
            if (!error.ok()) {
                qDebug() << "WebRTCAudioSender: 设置本地描述失败:" << error.message();
            } else {
                qDebug() << "WebRTCAudioSender: 设置本地描述成功";
            }
        }
    };
    
    // 先获取 SDP 字符串，再转移所有权
    std::string sdp;
    desc->ToString(&sdp);
    QString offer = QString::fromStdString(sdp);
    
    rtc::scoped_refptr<webrtc::SetLocalDescriptionObserverInterface> localObserver =
        new rtc::RefCountedObject<SetLocalDescObserver>();
    
    std::unique_ptr<webrtc::SessionDescriptionInterface> ownedDesc(desc);
    m_peerConnection->SetLocalDescription(std::move(ownedDesc), localObserver);
    
    qDebug() << "WebRTCAudioSender: Offer SDP:" << offer;
    
    // 通过 SRS API 交换 SDP
    exchangeSdpWithSrs(offer);
}

void WebRTCAudioSender::onCreateOfferFailure(const webrtc::RTCError& error)
{
    qDebug() << "WebRTCAudioSender: 创建 Offer 失败:" << error.message();
    emit errorOccurred(QString("创建 Offer 失败: %1").arg(error.message()));
}

// 无需恢复 Qt 宏（此文件中未修改）
