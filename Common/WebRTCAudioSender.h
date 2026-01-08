#pragma once

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

/**
 * WebRTC 音频发送器
 * 用于通过 WebRTC 协议将音频数据推送到 SRS 服务器
 * 
 * 注意：此功能已禁用。现在语音功能直接使用浏览器实现，不再需要本地 WebRTC 接口。
 * 所有 WebRTC 相关方法已禁用，直接返回 false 或不执行任何操作。
 * 所有 WebRTC 相关的头文件、类继承和成员变量已移除。
 */
class WebRTCAudioSender : public QObject
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

    // PeerConnectionObserver 接口实现已移除（WebRTC 功能已禁用）

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
    /**
     * 初始化 WebRTC 连接（已禁用）
     */
    bool initializeWebRTC();

    /**
     * 清理 WebRTC 资源（已禁用）
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
    
    // 音频输入（已禁用，保留用于兼容性）
    QAudioInput* m_audioInput = nullptr;
    QIODevice* m_audioInputDevice = nullptr;
    
    // WebRTC 相关成员变量已移除（功能已禁用）
    
    // 网络管理器（用于 SRS API 调用，已禁用）
    QNetworkAccessManager* m_networkManager = nullptr;

    // WebRTC 相关回调方法已移除（功能已禁用）
};
