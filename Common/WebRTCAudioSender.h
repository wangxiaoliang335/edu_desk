#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

// WebRTC 头文件优先，避免 Qt 宏干扰
#ifdef slots
#pragma push_macro("slots")
#undef slots
#define WEBRTC_AUDIO_SENDER_RESTORE_SLOTS 1
#endif
#ifdef signals
#pragma push_macro("signals")
#undef signals
#define WEBRTC_AUDIO_SENDER_RESTORE_SIGNALS 1
#endif
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#ifdef WEBRTC_AUDIO_SENDER_RESTORE_SLOTS
#pragma pop_macro("slots")
#undef WEBRTC_AUDIO_SENDER_RESTORE_SLOTS
#endif
#ifdef WEBRTC_AUDIO_SENDER_RESTORE_SIGNALS
#pragma pop_macro("signals")
#undef WEBRTC_AUDIO_SENDER_RESTORE_SIGNALS
#endif

class OfferObserver;

// Qt 头文件
#include <QObject>
#include <QString>
#include <QAudioInput>
#include <QIODevice>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <memory>
#include <vector>

namespace rtc {
template <class T>
class scoped_refptr;
class Thread;
}  // namespace rtc

namespace webrtc {
class PeerConnectionFactoryInterface;
class PeerConnectionInterface;
class AudioTrackInterface;
class AudioSourceInterface;
class DataChannelInterface;
class IceCandidateInterface;
class RTCError;
}  // namespace webrtc

/**
 * WebRTC 音频发送器
 * 用于通过 WebRTC 协议将音频数据推送到 SRS 服务器
 */
class WebRTCAudioSender : public QObject, 
                          public webrtc::PeerConnectionObserver
{
    Q_OBJECT

public:
    explicit WebRTCAudioSender(QObject* parent = nullptr);
    ~WebRTCAudioSender();

    /**
     * 连接到 SRS 服务器并开始发送音频
     * @param serverUrl SRS 服务器地址，例如 "webrtc://47.100.126.194:1985/live/stream1"
     */
    bool connectToServer(const QString& serverUrl);

    /**
     * 断开连接并停止发送音频
     */
    void disconnectFromServer();

    /**
     * 检查是否已连接
     */
    bool isConnected() const { return m_isConnected; }

    // PeerConnectionObserver 接口实现
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
    void OnRenegotiationNeeded() override;

signals:
    /**
     * 连接状态改变
     */
    void connectionStateChanged(bool connected);

    /**
     * 错误信号
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * 处理音频输入数据
     */
    void onAudioDataReady();

    /**
     * 处理 SRS API 响应
     */
    void onSrsApiReply(QNetworkReply* reply);

private:
    friend class OfferObserver;

    /**
     * 初始化 WebRTC 连接
     */
    bool initializeWebRTC();

    /**
     * 清理 WebRTC 资源
     */
    void cleanupWebRTC();

    /**
     * 初始化音频输入
     */
    void initAudioInput();

    /**
     * 清理音频输入
     */
    void cleanupAudioInput();

    /**
     * 发送音频数据到 WebRTC
     */
    void sendAudioData(const QByteArray& audioData);

    /**
     * 创建 PeerConnectionFactory
     */
    bool createPeerConnectionFactory();

    /**
     * 创建 PeerConnection
     */
    bool createPeerConnection();

    /**
     * 创建音频轨道并添加到 PeerConnection
     */
    bool createAndAddAudioTrack();

    /**
     * 创建 Offer 并通过 SRS API 获取 Answer
     */
    void createOfferAndConnect();

    /**
     * 解析 SRS 服务器 URL
     */
    bool parseServerUrl(const QString& url, QString& host, int& port, QString& streamName);

    /**
     * 通过 SRS HTTP API 交换 SDP
     */
    void exchangeSdpWithSrs(const QString& offer);

private:
    bool m_isConnected = false;
    QString m_serverUrl;
    QString m_srsHost;
    int m_srsPort = 1985;
    QString m_streamName;
    
    // 音频输入
    QAudioInput* m_audioInput = nullptr;
    QIODevice* m_audioInputDevice = nullptr;
    
    // WebRTC 相关
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_peerConnectionFactory;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peerConnection;
    rtc::scoped_refptr<webrtc::AudioTrackInterface> m_audioTrack;
    rtc::scoped_refptr<webrtc::AudioSourceInterface> m_audioSource;
    
    // WebRTC 线程
    std::unique_ptr<rtc::Thread> m_networkThread;
    std::unique_ptr<rtc::Thread> m_workerThread;
    std::unique_ptr<rtc::Thread> m_signalingThread;
    
    // 网络管理器（用于 SRS API 调用）
    QNetworkAccessManager* m_networkManager = nullptr;

    void onCreateOfferSuccess(webrtc::SessionDescriptionInterface* desc);
    void onCreateOfferFailure(const webrtc::RTCError& error);
};
