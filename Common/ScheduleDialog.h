#pragma once
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFrame>
#include <qfiledialog.h>
#include <qdebug.h>
#include <qlineedit.h>
#include <QWidget>
#include <QObject>
#include <QMouseEvent>
#include <QAudioInput>
#include <QAudioFormat>
#include <QIODevice>
#include <qprogressbar.h>
#include "CustomListDialog.h"
#include "ClickableLabel.h"
#include "TAHttpHandler.h"
#include "ChatDialog.h"
#include "CommonInfo.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

class ClickableWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ClickableWidget(QWidget* parent = nullptr) : QWidget(parent) {}

signals:
	void clicked();  // 点击信号

protected:
	void mousePressEvent(QMouseEvent* event) override
	{
		if (event->button() == Qt::LeftButton)
		{
			emit clicked();
		}
		QWidget::mousePressEvent(event); // 可选：让父类继续处理事件
	}
};

class ScheduleDialog : public QDialog
{
	Q_OBJECT
public:
	ScheduleDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
	{
		setWindowTitle("课程表");
		resize(700, 500);
		setStyleSheet("QPushButton { font-size:14px; } QLabel { font-size:14px; }");
		m_taHttpHandler = new TAHttpHandler();

		m_pWs = pWs;

		m_chatDlg = new ChatDialog(this, m_pWs);
		customListDlg = new CustomListDialog(this);
		QVBoxLayout* mainLayout = new QVBoxLayout(this);

		// 顶部：头像 + 班级信息 + 功能按钮 + 更多
		QHBoxLayout* topLayout = new QHBoxLayout;
		ClickableLabel* lblAvatar = new ClickableLabel();
		lblAvatar->setFixedSize(50, 50);

		QPixmap avatarPixmap(".\\res\\img\\home.png");
		// 如果需要缩放到控件大小：
		avatarPixmap = avatarPixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		lblAvatar->setPixmap(avatarPixmap);
		lblAvatar->setScaledContents(true); // 自动适应 QLabel 尺寸

		lblAvatar->setStyleSheet("background-color: lightgray; border:1px solid gray; text-align:center;");
		connect(lblAvatar, &ClickableLabel::clicked, this, [&, lblAvatar]() {
			QString file = QFileDialog::getOpenFileName(
				this, "选择新头像", "", "Images (*.png *.jpg *.jpeg *.bmp)");
			if (!file.isEmpty()) {
				lblAvatar->setPixmap(QPixmap(file));
				uploadAvatar(file);
			}
			});

		m_lblClass = new QLabel("");
		QPushButton* btnEdit = new QPushButton("✎");
		btnEdit->setFixedSize(24, 24);

		QPushButton* btnSeat = new QPushButton("座次表");
		QPushButton* btnCam = new QPushButton("摄像头");
		btnTalk = new QPushButton("按住开始对讲");
		QPushButton* btnMsg = new QPushButton("通知");
		QPushButton* btnTask = new QPushButton("作业");
		QString greenStyle = "background-color: green; color: white; padding: 4px 8px;";
		btnSeat->setStyleSheet(greenStyle);
		btnCam->setStyleSheet(greenStyle);
		btnTalk->setStyleSheet(greenStyle);
		btnMsg->setStyleSheet(greenStyle);
		btnTask->setStyleSheet(greenStyle);

		QPushButton* btnMore = new QPushButton("...");
		btnMore->setFixedSize(48, 24);
		btnMore->setText("...");
		btnMore->setStyleSheet(
			"QPushButton {"
			"background-color: transparent;"
			"color: black;"
			"font-weight: bold;"
			"font-size: 16px;"
			"border: none;"
			"}"
			"QPushButton:hover {"
			"color: black;"
			"background-color: transparent;"
			"}"
		);

		connect(btnMore, &QPushButton::clicked, this, [=]() {
			if (customListDlg && customListDlg->isHidden())
			{
				customListDlg->show();
			}
			else if (customListDlg && !customListDlg->isHidden())
			{
				customListDlg->hide();
			}
			});

		topLayout->addWidget(lblAvatar);
		topLayout->addWidget(m_lblClass);
		topLayout->addWidget(btnEdit);
		topLayout->addSpacing(10);
		topLayout->addWidget(btnSeat);
		topLayout->addWidget(btnCam);
		topLayout->addWidget(btnTalk);
		topLayout->addWidget(btnMsg);
		topLayout->addWidget(btnTask);
		topLayout->addStretch();
		topLayout->addWidget(btnMore);
		mainLayout->addLayout(topLayout);

		// 时间 + 科目行
		QHBoxLayout* timeLayout = new QHBoxLayout;
		QString timeStyle = "background-color: royalblue; color: white; font-size:12px; min-width:40px;";
		QString subjectStyle = "background-color: royalblue; color: white; font-size:12px; min-width:50px;";

		QStringList times = { "7:20","8:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00","2:00" };
		QStringList subs = { "晨读","语文","数学","英语","物理","午饭","午休","数学","美术","道法","课服","晚自习","" };

		QVBoxLayout* vTimes = new QVBoxLayout;
		QHBoxLayout* hTimeRow = new QHBoxLayout;
		QHBoxLayout* hSubRow = new QHBoxLayout;

		for (int i = 0; i < times.size(); ++i) {
			QPushButton* btnT = new QPushButton(times[i]);
			btnT->setStyleSheet(timeStyle);
			hTimeRow->addWidget(btnT);

			QPushButton* btnS = new QPushButton(subs[i]);
			btnS->setStyleSheet(subjectStyle);
			hSubRow->addWidget(btnS);
		}

		vTimes->addLayout(hTimeRow);
		vTimes->addLayout(hSubRow);
		// 包一层方便加边框
		QFrame* frameTimes = new QFrame;
		frameTimes->setLayout(vTimes);
		frameTimes->setFrameShape(QFrame::StyledPanel);
		mainLayout->addWidget(frameTimes);

		// 红色分隔线与时间箭头
		QFrame* line = new QFrame;
		line->setFrameShape(QFrame::HLine);
		line->setStyleSheet("color: red; border: 1px solid red;");
		mainLayout->addWidget(line);

		QHBoxLayout* timeIndicatorLayout = new QHBoxLayout;
		QLabel* lblArrow = new QLabel("↓");
		QLabel* lblTime = new QLabel("12:10");
		lblTime->setAlignment(Qt::AlignCenter);
		lblTime->setFixedSize(50, 25);
		lblTime->setStyleSheet("background-color: pink; color:red; font-weight:bold;");
		timeIndicatorLayout->addWidget(lblArrow);
		timeIndicatorLayout->addWidget(lblTime);
		timeIndicatorLayout->addStretch();
		mainLayout->addLayout(timeIndicatorLayout);

		// 表格区域
		QTableWidget* table = new QTableWidget(5, 8);
		table->horizontalHeader()->setVisible(false);
		table->verticalHeader()->setVisible(false);
		table->setStyleSheet("QTableWidget { gridline-color:blue; } QHeaderView::section { background-color:blue; }");
		mainLayout->addWidget(table);

		// 红框消息输入栏
		QHBoxLayout* inputLayout = new QHBoxLayout;

		QPushButton* btnVoice = new QPushButton("🔊");
		btnVoice->setFixedSize(30, 30);

		QLineEdit* editMessage = new QLineEdit();
		editMessage->setPlaceholderText("请输入消息...");
		editMessage->setMinimumHeight(30);
		editMessage->setEnabled(false);

		QPushButton* btnEmoji = new QPushButton("😊");
		btnEmoji->setFixedSize(30, 30);

		QPushButton* btnPlus = new QPushButton("➕");
		btnPlus->setFixedSize(30, 30);

		inputLayout->addStretch(1);
		inputLayout->addWidget(btnVoice);
		inputLayout->addWidget(editMessage, 1);
		inputLayout->addWidget(btnEmoji);
		inputLayout->addWidget(btnPlus);
		inputLayout->addStretch(1);

		ClickableWidget* inputWidget = new ClickableWidget();
		inputWidget->setLayout(inputLayout);
		inputWidget->setStyleSheet("background-color: white; border: 1px solid red;");

		// 绑定点击事件
		connect(inputWidget, &ClickableWidget::clicked, this, [=]() {
			qDebug() << "红框区域被点击！";
			// 这里可以弹出输入框、打开聊天功能等
			if (m_chatDlg)
			{
				m_chatDlg->show();
			}
			});

		mainLayout->addWidget(inputWidget);

		// 黄色圆圈数字
		QLabel* lblNum = new QLabel("3");
		lblNum->setAlignment(Qt::AlignCenter);
		lblNum->setFixedSize(30, 30);
		lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
		mainLayout->addWidget(lblNum, 0, Qt::AlignRight);

		//// ===== 音量进度条 =====
		//m_volumeBar = new QProgressBar();
		//m_volumeBar->setRange(0, 100);
		//m_volumeBar->setValue(0);
		//m_volumeBar->setTextVisible(false);
		//m_volumeBar->setStyleSheet(
		//	"QProgressBar { height:8px; border:1px solid gray; background:lightgray; }"
		//	"QProgressBar::chunk { background-color: red; }");
		//mainLayout->addWidget(m_volumeBar);


		//// 底部右下角黄色圆圈数字
		//QLabel* lblNum = new QLabel("3");
		//lblNum->setAlignment(Qt::AlignCenter);
		//lblNum->setFixedSize(30, 30);
		//lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
		//mainLayout->addWidget(lblNum, 0, Qt::AlignRight);

		// 信号与槽连接（C++11 lambda）
		//connect(btnTalk, &QPushButton::clicked, this, &ScheduleDialog::onBtnTalkClicked);
		// 按下按钮 -> 开始采集
		connect(btnTalk, &QPushButton::pressed, this, [=]() {
			pressStartMs = QDateTime::currentMSecsSinceEpoch();
			btnTalk->setStyleSheet("background-color: red; color: white; padding: 4px 8px; font-size:14px;");
			btnTalk->setText("录音中...松开结束");
			qDebug() << "开始对讲（按钮按下）";
			start();
			//btnTalk->setText("松开结束对讲");
		});

		// 松开按钮 -> 停止采集
		connect(btnTalk, &QPushButton::released, this, [=]() {
			/*stop();
			btnTalk->setText("按住开始对讲");*/
			qint64 releaseMs = QDateTime::currentMSecsSinceEpoch();
			qint64 duration = releaseMs - pressStartMs;

			stop();  // 停止采集

			if (duration < 500) {
				qDebug() << "录音时间过短(" << duration << "ms)，丢弃";
				// 这里可以直接 return 或做短音频忽略逻辑
			}
			else {
				qDebug() << "录音完成，时长:" << duration << "ms，已发送音频";
			}

			btnTalk->setStyleSheet("background-color: green; color: white; padding: 4px 8px; font-size:14px;");
			btnTalk->setText("按住开始对讲");
		});
	}

	void uploadAvatar(QString filePath)
	{
		// ===== 1. 读取头像图片 =====
		QFile file(filePath);  // 本地头像路径
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "Failed to open image file.";
			return;
		}
		QByteArray imageData = file.readAll(); // 二进制数据
		file.close();

		// ===== 2. 图片转 Base64 =====
		QString imageBase64 = QString::fromLatin1(imageData.toBase64());

		// ===== 3. 构造 JSON 数据 =====
		QMap<QString, QString> params;
		params["avatar"] = imageBase64;
		params["unique_group_id"] = m_unique_group_id;
		if (m_taHttpHandler)
		{
			m_taHttpHandler->post(QString("http://47.100.126.194:5000/updateGroupInfo"), params);
		}
	}

	void InitWebSocket()
	{
		if (m_chatDlg)
		{
			m_chatDlg->InitWebSocket();
		}
	}

	void setNoticeMsg(QList<Notification> listNoticeMsg)
	{
		if (m_chatDlg)
		{
			m_chatDlg->setNoticeMsg(listNoticeMsg);
		}
	}

	void InitData(QString groupName, QString unique_group_id, bool iGroupOwner)
	{
		m_groupName = groupName;
		m_unique_group_id = unique_group_id;
		m_iGroupOwner = iGroupOwner;
		if (m_lblClass)
		{
			m_lblClass->setText(groupName);
		}
		m_chatDlg->InitData(m_unique_group_id, iGroupOwner);
		UserInfo userInfo = CommonInfo::GetData();
		m_userId = userInfo.teacher_unique_id;
		m_userName = userInfo.strName;
	}

	void start() {
		// 枚举可用输入设备
		QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
		qDebug() << "===== 可用麦克风设备列表 =====";
		for (auto& dev : devices) {
			qDebug() << "设备:" << dev.deviceName();
		}

		// 准备采样格式
		QAudioFormat fmt;
		fmt.setSampleRate(44100);
		fmt.setChannelCount(2);
		fmt.setSampleSize(16);
		fmt.setCodec("audio/pcm");
		fmt.setByteOrder(QAudioFormat::LittleEndian);
		fmt.setSampleType(QAudioFormat::SignedInt);

		// 检查当前默认设备
		QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
		qDebug() << "默认输入设备:" << info.deviceName();

		// 如果不支持，回退到最近格式
		if (!info.isFormatSupported(fmt)) {
			qWarning() << "当前设备不支持 44100Hz 立体声 S16 格式, 使用 nearestFormat";
			fmt = info.nearestFormat(fmt);
		}

		// 打印最终使用的格式
		qDebug() << "使用格式:"
			<< fmt.sampleRate() << "Hz"
			<< fmt.channelCount() << "声道"
			<< fmt.sampleSize() << "bit"
			<< fmt.codec();

		// 创建输入实例
		audioInput = new QAudioInput(info, fmt, this);

		// 让 readyRead 提前触发
		audioInput->setBufferSize(4096);

		// 初始化编码器
		initEncoder();

		// 启动采集
		inputDevice = audioInput->start();
		if (!inputDevice) {
			qCritical() << "❌ AudioInput start() 失败，可能是系统权限或设备问题";
			return;
		}
		else {
			qDebug() << "✅ AudioInput 已启动，等待 readyRead 事件...";
		}

		// 绑定 readyRead
		connect(inputDevice, &QIODevice::readyRead, this, &ScheduleDialog::onReadyRead);

		// 额外定时器监控（可选）
		QTimer::singleShot(3000, this, [=]() {
			if (audioInput && audioInput->state() != QAudio::ActiveState) {
				qWarning() << "⚠️ AudioInput 未处于 ActiveState, 当前状态:" << audioInput->state();
			}
		});
	}

	void stop() {
		if (audioInput) { audioInput->stop(); delete audioInput; audioInput = nullptr; }
		if (codecCtx) avcodec_free_context(&codecCtx);
		if (frame) av_frame_free(&frame);
		if (pkt) av_packet_free(&pkt);
		if (swrCtx) swr_free(&swrCtx);
	}

	void encodeAndSend(const QByteArray& pcm) {
		int16_t* pcmData = (int16_t*)pcm.data();
		int numSamples = pcm.size() / (2 * codecCtx->channels);
		const uint8_t* inData[1] = { (uint8_t*)pcmData };
		swr_convert(swrCtx, frame->data, frame->nb_samples, inData, numSamples);

		if (avcodec_send_frame(codecCtx, frame) >= 0) {
			while (avcodec_receive_packet(codecCtx, pkt) == 0) {
				QByteArray aacData((char*)pkt->data, pkt->size);

				QByteArray packet;
				QDataStream ds(&packet, QIODevice::WriteOnly);
				ds.setByteOrder(QDataStream::LittleEndian);

				// ===== 打包帧 =====
				quint8 frameType = 6; // 音频帧
				ds << frameType;

				QByteArray groupIdBytes = m_unique_group_id.toUtf8();
				quint32 groupIdLen = groupIdBytes.size();
				ds << groupIdLen;
				ds.writeRawData(groupIdBytes.constData(), groupIdLen);

				QByteArray senderIdBytes = m_userId.toUtf8();
				quint32 senderIdLen = senderIdBytes.size();
				ds << senderIdLen;
				ds.writeRawData(senderIdBytes.constData(), senderIdLen);

				QByteArray senderNameBytes = m_userName.toUtf8();
				quint32 senderNameLen = senderNameBytes.size();
				ds << senderNameLen;
				ds.writeRawData(senderNameBytes.constData(), senderNameLen);

				quint64 ts = QDateTime::currentMSecsSinceEpoch();
				ds << ts;

				quint32 aacLen = aacData.size();
				ds << aacLen;
				ds.writeRawData(aacData.constData(), aacLen);

				//m_ws.sendBinaryMessage(packet);
				TaQTWebSocket::sendBinaryMessage(packet);
				// ===== 完成 =====
				av_packet_unref(pkt);
			}
		}
	}

	void initEncoder() {
		// 不再调用 avcodec_register_all()
		// 新版已自动注册
		av_log_set_level(AV_LOG_INFO);
		const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		codecCtx = avcodec_alloc_context3(codec);
		codecCtx->bit_rate = 128000;
		codecCtx->sample_rate = 44100;
		codecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
		codecCtx->channels = 2;
		codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;

		if (avcodec_open2(codecCtx, codec, NULL) < 0) qFatal("AAC encoder open fail");

		frame = av_frame_alloc();
		frame->nb_samples = codecCtx->frame_size;
		frame->format = codecCtx->sample_fmt;
		frame->channel_layout = codecCtx->channel_layout;
		av_frame_get_buffer(frame, 0);

		pkt = av_packet_alloc();

		swrCtx = swr_alloc_set_opts(NULL,
			codecCtx->channel_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
			codecCtx->channel_layout, AV_SAMPLE_FMT_S16, codecCtx->sample_rate,
			0, NULL);
		swr_init(swrCtx);
	}

private slots:
	void onBtnTalkClicked() {
		qDebug() << "按钮被点击了!";
		if (false == m_isBeginTalk)
		{
			start();
			btnTalk->setText("停止对讲");
			m_isBeginTalk = true;
		}
		else
		{
			stop();
			btnTalk->setText("开始对讲");
			m_isBeginTalk = false;
		}
	}

	void onReadyRead() {
		QByteArray pcm = inputDevice->readAll();
		encodeAndSend(pcm);

		//// ===== 计算音量幅度 =====
		//const int16_t* samples = reinterpret_cast<const int16_t*>(pcm.constData());
		//int sampleCount = pcm.size() / 2; // 16 bit 每样本2字节
		//double sumSquares = 0;
		//for (int i = 0; i < sampleCount; ++i) {
		//	sumSquares += samples[i] * samples[i];
		//}
		//double rms = sqrt(sumSquares / qMax(sampleCount, 1));
		//double normalized = rms / 32768.0;  // 归一化到 0-1

		//// 转百分比 (0–100)，加入限制，防止跳动过猛
		//int volume = int(normalized * 100);
		//volume = qBound(0, volume, 100);

		//// 更新音量条
		//m_volumeBar->setValue(volume);
	}

private:
	CustomListDialog* customListDlg = NULL;
	QLabel* m_lblClass = NULL;
	QString m_groupName;
	QString m_unique_group_id;
	TAHttpHandler* m_taHttpHandler = NULL;
	ChatDialog* m_chatDlg = NULL;
	TaQTWebSocket* m_pWs = NULL;
	bool m_iGroupOwner = false;
	QAudioInput* audioInput = nullptr;
	QIODevice* inputDevice = nullptr;
	AVCodecContext* codecCtx = nullptr;
	AVFrame* frame = nullptr;
	AVPacket* pkt = nullptr;
	SwrContext* swrCtx = nullptr;
	QString m_userId;
	QString m_userName;
	QPushButton* btnTalk = NULL;
	bool m_isBeginTalk = false;
	// 用于记录按下开始时间
	qint64 pressStartMs = 0;
	//QProgressBar* m_volumeBar = nullptr;
};
