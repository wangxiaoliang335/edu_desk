#include "AudioReceiver.h"
#include <QDataStream>
#include <QDir>
#include <cmath>

AudioReceiver::AudioReceiver(QObject* parent, TaQTWebSocket* pWs)
    : QObject(parent)
{
    initAudioOutput();
    initFFmpegDecoder();

    if (pWs) attachWebSocket(pWs);
}

AudioReceiver::~AudioReceiver()
{
    if (audioOutput) { delete audioOutput; audioOutput = nullptr; }
    if (outputDevice) outputDevice = nullptr;

    avcodec_free_context(&codecCtx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swrCtx);
}

void AudioReceiver::attachWebSocket(TaQTWebSocket* ws)
{
    m_pWs = ws;
    if (m_pWs)
    {
        connect(m_pWs, &TaQTWebSocket::newBinaryMessage,
            this, &AudioReceiver::onBinaryMessageReceived);
        qDebug() << "✅ AudioReceiver attached to WebSocket";
    }
}

void AudioReceiver::initAudioOutput()
{
    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);               // 输出单声道
    fmt.setSampleSize(16);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleType(QAudioFormat::SignedInt);

    audioOutput = new QAudioOutput(QAudioDeviceInfo::defaultOutputDevice(), fmt, this);
    outputDevice = audioOutput->start();
    qDebug() << "🎧 AudioOutput initialized (44100Hz, mono, 16bit)";
}

void AudioReceiver::initFFmpegDecoder()
{
    av_log_set_level(AV_LOG_ERROR);

    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec)
    {
        qWarning() << "❌ AAC decoder not found!";
        return;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx)
    {
        qWarning() << "❌ Could not allocate decoder context!";
        return;
    }

    codecCtx->thread_count = 2;

    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        qWarning() << "❌ Could not open AAC decoder!";
        return;
    }

    frame = av_frame_alloc();
    pkt = av_packet_alloc();
    qDebug() << "✅ FFmpeg AAC decoder initialized";
}

void AudioReceiver::onBinaryMessageReceived(const QByteArray& msg)
{
    QDataStream ds(msg);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint8 frameType;
    ds >> frameType;
    if (frameType != 6) return; // 非音频帧忽略

    quint8 flag;
    ds >> flag;

    quint32 groupIdLen;
    ds >> groupIdLen;
    QByteArray groupIdBytes(groupIdLen, 0);
    ds.readRawData(groupIdBytes.data(), groupIdLen);

    quint32 senderIdLen;
    ds >> senderIdLen;
    QByteArray senderIdBytes(senderIdLen, 0);
    ds.readRawData(senderIdBytes.data(), senderIdLen);

    quint32 senderNameLen;
    ds >> senderNameLen;
    QByteArray senderNameBytes(senderNameLen, 0);
    ds.readRawData(senderNameBytes.data(), senderNameLen);

    QString senderName(senderNameBytes);

    quint64 timestamp;
    ds >> timestamp;

    quint32 aacLen;
    ds >> aacLen;
    QByteArray aacBytes(aacLen, 0);
    ds.readRawData(aacBytes.data(), aacLen);

    qDebug() << QString("🎧 Recv audio from [%1], len=%2, flag=%3")
        .arg(QString(senderNameBytes))
        .arg(aacLen)
        .arg(flag);

    // 处理文件保存逻辑
    if (flag == 0) {
        qDebug() << "▶️ 开始对讲";
        QString fileName = QCoreApplication::applicationDirPath() + "/recv_audio_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".aac";
        m_aacFile.setFileName(fileName);
        if (m_aacFile.open(QIODevice::WriteOnly)) {
            m_isRecording = true;
            qDebug() << "💾 开始保存 AAC 文件:" << fileName;
            if (aacLen > 0) m_aacFile.write(aacBytes);
        }
        else {
            qWarning() << "❌ 无法创建 AAC 保存文件";
        }
    }
    else if (flag == 1) {
        // 如果还没开始保存，则启动
        if (!m_isRecording) {
            QString fileName = QCoreApplication::applicationDirPath() + "/recv_audio_" +
                QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".aac";
            m_aacFile.setFileName(fileName);
            if (m_aacFile.open(QIODevice::WriteOnly)) {
                m_isRecording = true;
                qDebug() << "💾 开始保存 AAC 文件(从flag=1):" << fileName;
            }
        }
        if (m_isRecording && aacLen > 0) m_aacFile.write(aacBytes);
    }
    else if (flag == 2) {
        qDebug() << "⏹️ 结束对讲";
        if (m_isRecording) {
            if (aacLen > 0) m_aacFile.write(aacBytes);
            m_aacFile.close();
            m_isRecording = false;
            qDebug() << "💾 AAC 文件已保存完成:" << m_aacFile.fileName();
        }
    }

    if (aacLen > 0)
        decodeAndPlay(aacBytes);
}

void AudioReceiver::decodeAndPlay(const QByteArray& aac)
{
    if (!codecCtx || !frame || !pkt)
        return;

    av_packet_unref(pkt);
    pkt->data = (uint8_t*)aac.constData();
    pkt->size = aac.size();

    // 送入解码器
    int ret = avcodec_send_packet(codecCtx, pkt);
    if (ret < 0)
    {
        qWarning() << "⚠️ avcodec_send_packet failed" << ret;
        return;
    }

    while (true)
    {
        ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        if (ret < 0)
        {
            qWarning() << "⚠️ avcodec_receive_frame failed";
            break;
        }

        if (!swrCtx)
        {
            // 初始化 swrCtx 使用实际参数
            int64_t in_layout = frame->channel_layout;
            if (!in_layout)  // 某些 AAC 缺 layout
                in_layout = av_get_default_channel_layout(frame->channels);

            swrCtx = swr_alloc_set_opts(nullptr,
                AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, 44100,  // 输出：单声道、16位
                in_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                0, nullptr);
            if (!swrCtx || swr_init(swrCtx) < 0)
            {
                qWarning() << "❌ Init swrCtx failed!";
                return;
            }
            qDebug() << "🎚 swrCtx initialized  input_channels=" << frame->channels
                << " sample_fmt=" << av_get_sample_fmt_name((AVSampleFormat)frame->format);
        }

        if (frame->nb_samples <= 0)
        {
            qWarning() << "⚠️ frame->nb_samples = 0";
            continue;
        }

        int out_samples = av_rescale_rnd(
            swr_get_delay(swrCtx, frame->sample_rate) + frame->nb_samples,
            44100, frame->sample_rate, AV_ROUND_UP);

        QByteArray pcmBuf(out_samples * 2, 0); // 单声道 16bit
        uint8_t* outData[1] = { (uint8_t*)pcmBuf.data() };

        int converted = swr_convert(swrCtx, outData, out_samples,
            (const uint8_t**)frame->data, frame->nb_samples);

        if (converted < 0)
        {
            qWarning() << "⚠️ swr_convert failed";
            continue;
        }

        if (outputDevice)
            outputDevice->write(pcmBuf);
    }
}
