#pragma once

#include <QObject>
#include <QAudioOutput>
#include <QIODevice>
#include <QDebug>
#include <QByteArray>
#include <QCoreApplication>
#include <qfile.h>
#include "TaQTWebSocket.h"

// FFmpeg
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

class AudioReceiver : public QObject
{
    Q_OBJECT
public:
    explicit AudioReceiver(QObject* parent = nullptr, TaQTWebSocket* pWs = nullptr);
    ~AudioReceiver();

    void attachWebSocket(TaQTWebSocket* ws);
    void initAudioOutput();
    void initFFmpegDecoder();

private slots:
    void onBinaryMessageReceived(const QByteArray& msg);

private:
    void decodeAndPlay(const QByteArray& aac);

private:
    QAudioOutput* audioOutput = nullptr;
    QIODevice* outputDevice = nullptr;

    AVCodecContext* codecCtx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    SwrContext* swrCtx = nullptr;
    TaQTWebSocket* m_pWs = nullptr;

private:
    QFile m_aacFile;      // 用于保存AAC文件
    bool m_isRecording = false;   // 当前是否在录音保存状态

};
