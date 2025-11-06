#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QList>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTimer>
#include <QDateTime>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonParseError>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslError>
#include <QStandardPaths>
#include <QDir>
#include <QProgressBar>
#include <QMap>
#include <QPair>
#include "TaQTWebSocket.h"
#include "CommonInfo.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "ImSDK/includes/TIMCloudCallback.h"

struct ChatMessage {
    QString avatarPath;
    QString senderName;
    QString messageText;
    bool isMine;
    QDateTime time;
};

class ClickableLabelEx : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabelEx(QWidget* parent = nullptr) : QLabel(parent) {}
signals:
    void clicked();
    void doubleClicked();
protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit clicked();
        QLabel::mousePressEvent(event);
    }
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        emit doubleClicked();
        QLabel::mouseDoubleClickEvent(event);
    }
};

class ChatDialog : public QDialog
{
    Q_OBJECT
public:
    ChatDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
    {
        setWindowTitle("多类型聊天对话框");
        resize(500, 660);

        m_pWs = pWs;

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        // 消息列表
        m_listWidget = new QListWidget();
        m_listWidget->setStyleSheet("QListWidget { background-color: #F5F5F5; border:none; }");
        mainLayout->addWidget(m_listWidget, 1);

        // 输入区
        QHBoxLayout* inputLayout = new QHBoxLayout();
        m_lineEdit = new QLineEdit();
        m_lineEdit->setPlaceholderText("请输入文字...");
        m_lineEdit->setMinimumHeight(36);

        QPushButton* btnSend = new QPushButton("发送");
        btnSend->setFixedSize(60, 36);

        QPushButton* btnImage = new QPushButton("📷");
        btnImage->setFixedSize(36, 36);

        QPushButton* btnFile = new QPushButton("📎");
        btnFile->setFixedSize(36, 36);

        QPushButton* btnVoice = new QPushButton("🎤");
        btnVoice->setFixedSize(36, 36);

        inputLayout->addWidget(m_lineEdit);
        inputLayout->addWidget(btnSend);
        inputLayout->addWidget(btnImage);
        inputLayout->addWidget(btnFile);
        inputLayout->addWidget(btnVoice);
        mainLayout->addLayout(inputLayout);

        connect(btnSend, &QPushButton::clicked, this, &ChatDialog::sendMyTextMessage);
        connect(m_lineEdit, &QLineEdit::returnPressed, this, &ChatDialog::sendMyTextMessage);
        connect(btnImage, &QPushButton::clicked, this, &ChatDialog::sendMyImageMessage);
        connect(btnFile, &QPushButton::clicked, this, &ChatDialog::sendMyFileMessage);
        connect(btnVoice, &QPushButton::clicked, this, &ChatDialog::sendMyVoiceMessage);

        // 测试对话
        addTextMessage(":/res/img/home.png", "班主任", "李老师，今天家里有事，我们调一下课吧", false);
        addTextMessage(":/res/img/home.png", "语文老师", "可以", false);
    }

    void InitData(QString unique_group_id, bool iGroupOwner)
    {
        m_unique_group_id = unique_group_id;
        m_iGroupOwner = iGroupOwner;
        
        // 注册到静态实例列表
        registerInstance();
        
        // 加载历史消息
        loadHistoryMessages();
    }
    
    ~ChatDialog()
    {
        // 从静态实例列表中移除
        unregisterInstance();
    }

    void InitWebSocket()
    {
        /*TaQTWebSocket::regRecvDlg(this);
        if (m_pWs)
        {
            connect(m_pWs, &TaQTWebSocket::newMessage,
                this, &ChatDialog::onWebSocketMessage);
        }*/
    }

    void setNoticeMsg(QList<Notification> listNoticeMsg)
    {
        UserInfo userInfo = CommonInfo::GetData();
        for (auto iter : listNoticeMsg)
        {
            if (QString::number(iter.sender_id) == userInfo.teacher_unique_id)
            {
                addTextMessage(":/res/img/home.png", iter.sender_name, iter.content, true);
            }
            else
            {
                addTextMessage(":/res/img/home.png", iter.sender_name, iter.content, false);
            }
        }
    }
    
    // 静态方法：提前注册消息回调（应在登录前调用，确保能接收到离线消息）
    static void ensureCallbackRegistered()
    {
        static bool s_callbackRegistered = false;
        if (!s_callbackRegistered) {
            TIMAddRecvNewMsgCallback(staticRecvNewMsgCallback, nullptr);
            s_callbackRegistered = true;
            qDebug() << "ChatDialog: 消息接收回调已注册（登录前）";
        }
    }

private slots:
    void sendMyTextMessage()
    {
        QString text = m_lineEdit->text().trimmed();
        if (text.isEmpty() || m_unique_group_id.isEmpty()) return;

        UserInfo userinfo = CommonInfo::GetData();
        
        // 先显示在界面上（乐观更新）
        addTextMessage(":/res/img/home.png", userinfo.strName, text, true);
        m_lineEdit->clear();

        // 使用腾讯SDK发送消息
        sendTextMessageViaTIMSDK(text, userinfo);
    }

    //// ChatDialog.cpp
    //void onWebSocketMessage(const QString& msg)
    //{
    //    qDebug() << " ClassTeacherDialog msg:" << msg; // 发信号;
    //    // 这里可以解析 JSON 或直接追加到聊天窗口
    //    //addTextMessage(":/avatar_teacher.png", "对方", msg, false);
    //    //m_NoticeMsg.push_back(msg);

    //    QJsonParseError parseError;
    //    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);

    //    if (parseError.error != QJsonParseError::NoError) {
    //        qDebug() << "JSON parse error:" << parseError.errorString();
    //    }
    //    else {
    //        if (doc.isObject()) {
    //            QJsonObject obj = doc.object();
    //            if (obj["data"].isArray())
    //            {
    //                // 4. 取出数组
    //                QJsonArray arr = obj["data"].toArray();
    //                // 5. 遍历数组
    //                for (const QJsonValue& value : arr) {
    //                    if (value.isObject()) { // 每个元素是一个对象
    //                        QJsonObject obj = value.toObject();
    //                        int qSender_id = obj["sender_id"].toInt();
    //                        QString strSender_id = QString("%1").arg(qSender_id, 6, 10, QChar('0'));
    //                        QString SenderName = obj["sender_name"].toString();
    //                        int iContent_text = obj["content_text"].toInt();
    //                        QString updated_at = obj["updated_at"].toString();
    //                        int is_agreed = obj["is_agreed"].toInt();
    //                        QString content = obj["content"].toString();
    //                        QString GroupName = obj["group_name"].toString();
    //                        if (1 != iContent_text && 2 != iContent_text && 3 != iContent_text && 4 != iContent_text)
    //                        {
    //                            addTextMessage(":/avatar_me.png", "我", content, true);
    //                        }
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}

    void sendMyImageMessage()
    {
        if (m_unique_group_id.isEmpty()) {
            QMessageBox::warning(this, "错误", "群组ID未设置，无法发送图片！");
            return;
        }
        
        UserInfo userinfo = CommonInfo::GetData();
        QString imgPath = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!imgPath.isEmpty()) {
            // 先显示在界面上
            addImageMessage(":/res/img/home.png", userinfo.strName, imgPath, true);
            // 使用腾讯SDK发送图片
            sendImageMessageViaTIMSDK(imgPath, userinfo);
        }
    }

    void sendMyFileMessage()
    {
        if (m_unique_group_id.isEmpty()) {
            QMessageBox::warning(this, "错误", "群组ID未设置，无法发送文件！");
            return;
        }
        
        UserInfo userinfo = CommonInfo::GetData();
        QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
        if (!filePath.isEmpty()) {
            // 先显示在界面上（带进度条），进度条和标签在 addFileMessage 中创建
            QPair<QProgressBar*, QLabel*> progressWidgets = addFileMessageWithProgress(":/res/img/home.png", userinfo.strName, filePath, true);
            
            // 使用腾讯SDK发送文件（传入进度条和状态标签）
            sendFileMessageViaTIMSDK(filePath, userinfo, progressWidgets.first, progressWidgets.second);
        }
    }

    void sendMyVoiceMessage()
    {
        if (m_unique_group_id.isEmpty()) {
            QMessageBox::warning(this, "错误", "群组ID未设置，无法发送语音！");
            return;
        }
        
        UserInfo userinfo = CommonInfo::GetData();
        // TODO: 实现语音录制功能
        // 目前先模拟 8秒语音
        addVoiceMessage(":/res/img/home.png", userinfo.strName, 8, true);
        // TODO: 使用腾讯SDK发送语音
        // sendVoiceMessageViaTIMSDK(voicePath, userinfo, 8);
    }

private:
    QListWidget* m_listWidget;
    QLineEdit* m_lineEdit;
    ChatMessage m_lastMessage;
    bool m_hasLastMessage = false;

    void addTimeLabel(const QDateTime& time)
    {
        QListWidgetItem* timeItem = new QListWidgetItem(m_listWidget);
        QLabel* lblTime = new QLabel(time.toString("yyyy-MM-dd hh:mm"));
        lblTime->setAlignment(Qt::AlignCenter);
        lblTime->setStyleSheet("color: gray; font-size: 12px;");
        timeItem->setSizeHint(QSize(0, 25));
        m_listWidget->addItem(timeItem);
        m_listWidget->setItemWidget(timeItem, lblTime);
    }

    bool shouldHideAvatar(const QString& senderName, bool isMine, const QDateTime& now)
    {
        return m_hasLastMessage &&
            m_lastMessage.senderName == senderName &&
            m_lastMessage.isMine == isMine &&
            m_lastMessage.time.secsTo(now) <= 180;
    }

    //如果 hideAvatar = true 就不显示头像但仍显示发送者名字。
    QWidget* buildMessageWidget(const QString& avatarPath, const QString& senderName, QWidget* contentWidget, bool isMine, bool hideAvatar)
    {
        QWidget* msgWidget = new QWidget();
        QVBoxLayout* vLayout = new QVBoxLayout(msgWidget);
        vLayout->setContentsMargins(5, 5, 5, 5);
        vLayout->setSpacing(2);

        // ====== 第一行：发送者名称 ======
        QLabel* lblName = new QLabel(senderName);
        lblName->setStyleSheet("color: gray; font-size: 12px;");
        if (isMine) {
            lblName->setAlignment(Qt::AlignRight);
        }
        else {
            lblName->setAlignment(Qt::AlignLeft);
        }
        vLayout->addWidget(lblName);

        // ====== 第二行：头像 + 气泡 ======
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setContentsMargins(0, 0, 0, 0);

        QLabel* lblAvatar = new QLabel();
        lblAvatar->setFixedSize(36, 36);
        if (!hideAvatar)
        {
            QPixmap avatarPixmap;
            if (QFile::exists(avatarPath))
                avatarPixmap.load(avatarPath);
            else
                avatarPixmap.fill(Qt::gray);
            avatarPixmap = avatarPixmap.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            lblAvatar->setPixmap(avatarPixmap);
            lblAvatar->setStyleSheet("border-radius: 18px;");
        }

        if (isMine)
        {
            contentWidget->setStyleSheet("background-color: #A0E75A; color: black; border-radius: 12px; padding: 8px;");
            hLayout->addStretch();
            hLayout->addWidget(contentWidget);
            if (!hideAvatar) hLayout->addWidget(lblAvatar);
        }
        else
        {
            contentWidget->setStyleSheet("background-color: #EAEAEA; color: black; border-radius: 12px; padding: 8px;");
            if (!hideAvatar) hLayout->addWidget(lblAvatar);
            hLayout->addWidget(contentWidget);
            hLayout->addStretch();
        }

        vLayout->addLayout(hLayout);
        return msgWidget;
    }


    void addTextMessage(const QString& avatarPath, const QString& senderName, const QString& text, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QLabel* lblMessage = new QLabel(text);
        lblMessage->setWordWrap(true);
        lblMessage->setTextInteractionFlags(Qt::TextSelectableByMouse);

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, lblMessage, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, text, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    void addImageMessage(const QString& avatarPath, const QString& senderName, const QString& imgPath, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        ClickableLabelEx* lblImage = new ClickableLabelEx();
        QPixmap pix(imgPath);
        pix = pix.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        lblImage->setPixmap(pix);
        lblImage->setScaledContents(true);
        // 双击图片时用系统默认图片查看器打开
        connect(lblImage, &ClickableLabelEx::doubleClicked, this, [=]() {
            QFileInfo fileInfo(imgPath);
            if (fileInfo.exists()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(imgPath));
            } else {
                QMessageBox::warning(this, "错误", "图片文件不存在：" + imgPath);
            }
        });

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, lblImage, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[图片]", isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    // 添加带进度条的文件消息（用于发送文件）
    QPair<QProgressBar*, QLabel*> addFileMessageWithProgress(const QString& avatarPath, const QString& senderName, 
                                                             const QString& filePath, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QFileInfo fi(filePath);
        QString fileName = fi.fileName();
        qint64 fileSizeBytes = fi.size();
        QString fileSize = fileSizeBytes < 1024 ? QString("%1 字节").arg(fileSizeBytes) :
                          fileSizeBytes < 1024 * 1024 ? QString("%1 KB").arg(fileSizeBytes / 1024.0, 0, 'f', 1) :
                          QString("%1 MB").arg(fileSizeBytes / (1024.0 * 1024.0), 0, 'f', 1);

        // 创建文件消息容器
        QWidget* fileWidget = new QWidget();
        QVBoxLayout* fileLayout = new QVBoxLayout(fileWidget);
        fileLayout->setContentsMargins(5, 5, 5, 5);
        fileLayout->setSpacing(5);
        
        // 文件名和大小标签
        ClickableLabelEx* lblFile = new ClickableLabelEx(fileWidget);
        lblFile->setText(fileName + "\n" + fileSize);
        lblFile->setStyleSheet("qproperty-alignment: AlignCenter;");
        lblFile->setMinimumSize(120, 50);
        connect(lblFile, &ClickableLabelEx::clicked, this, [=]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        });
        fileLayout->addWidget(lblFile);
        
        // 创建进度条（设置父对象为fileWidget，确保生命周期）
        QProgressBar* progressBar = new QProgressBar(fileWidget);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setFormat("%p%");
        fileLayout->addWidget(progressBar);
        
        // 创建状态标签（设置父对象为fileWidget，确保生命周期）
        QLabel* statusLabel = new QLabel(fileWidget);
        statusLabel->setText("准备发送...");
        statusLabel->setStyleSheet("color: gray; font-size: 10px;");
        fileLayout->addWidget(statusLabel);

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, fileWidget, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[文件] " + fileName, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
        
        // 返回进度条和状态标签的指针
        return qMakePair(progressBar, statusLabel);
    }

    void addFileMessage(const QString& avatarPath, const QString& senderName, const QString& filePath, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QFileInfo fi(filePath);
        QString fileName = fi.fileName();
        QString fileSize = QString::number(fi.size() / 1024.0, 'f', 1) + " KB";

        ClickableLabelEx* lblFile = new ClickableLabelEx();
        lblFile->setText(fileName + "\n" + fileSize);
        lblFile->setStyleSheet("qproperty-alignment: AlignCenter;");
        lblFile->setMinimumSize(120, 50);
        connect(lblFile, &ClickableLabelEx::clicked, this, [=]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        });

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, lblFile, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[文件] " + fileName, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    void addVoiceMessage(const QString& avatarPath, const QString& senderName, int seconds, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        ClickableLabelEx* lblVoice = new ClickableLabelEx();
        lblVoice->setText(QString("🎵 语音 %1 s").arg(seconds));
        lblVoice->setMinimumSize(80 + seconds * 5, 36); // 秒数越多，宽度越长
        connect(lblVoice, &ClickableLabelEx::clicked, this, [=]() {
            // 这里可以接入真正的语音播放功能
            qDebug("播放语音 %d 秒", seconds);
            });

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, lblVoice, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, QString("[语音] %1秒").arg(seconds), isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    // 使用腾讯SDK发送文本消息
    void sendTextMessageViaTIMSDK(const QString& text, const UserInfo& userinfo)
    {
        if (m_unique_group_id.isEmpty()) return;
        
        // 构造消息元素
        QJsonObject textElem;
        textElem[kTIMElemType] = (int)kTIMElem_Text;
        textElem[kTIMTextElemContent] = text;
        
        // 构造消息
        QJsonObject msgObj;
        QJsonArray elemArray;
        elemArray.append(textElem);
        msgObj[kTIMMsgElemArray] = elemArray;
        msgObj[kTIMMsgSender] = userinfo.strUserId;
        qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        msgObj[kTIMMsgClientTime] = currentTime;
        msgObj[kTIMMsgServerTime] = currentTime;
        
        // 转换为JSON字符串
        QJsonDocument doc(msgObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 创建回调数据结构
        struct SendMsgCallbackData {
            ChatDialog* dlg;
            QString text;
            QString senderName;
        };
        SendMsgCallbackData* callbackData = new SendMsgCallbackData;
        callbackData->dlg = this;
        callbackData->text = text;
        callbackData->senderName = userinfo.strName;
        
        // 发送消息
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgSendNewMsg(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                SendMsgCallbackData* data = (SendMsgCallbackData*)user_data;
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "发送消息失败，错误码:" << code << "，描述:" << errorDesc;
                    QMessageBox::critical(data->dlg, "发送失败", QString("消息发送失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc));
                } else {
                    qDebug() << "消息发送成功";
                }
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgSendNewMsg失败，错误码:" << ret;
            QMessageBox::critical(this, "错误", QString("调用发送接口失败，错误码: %1").arg(ret));
            delete callbackData;
        }
    }
    
    // 使用腾讯SDK发送图片消息
    void sendImageMessageViaTIMSDK(const QString& imgPath, const UserInfo& userinfo)
    {
        if (m_unique_group_id.isEmpty() || imgPath.isEmpty()) return;
        
        // 检查文件是否存在
        QFileInfo fileInfo(imgPath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "错误", QString("图片文件不存在：%1").arg(imgPath));
            return;
        }
        
        // 将路径转换为本地路径格式（Windows需要使用反斜杠）
        QString normalizedPath = QDir::toNativeSeparators(imgPath);
        
        // 构造图片元素
        QJsonObject imageElem;
        imageElem[kTIMElemType] = (int)kTIMElem_Image;
        imageElem[kTIMImageElemOrigPath] = normalizedPath; // 图片路径（必填）
        imageElem[kTIMImageElemLevel] = (int)kTIMImageLevel_Compression; // 压缩图发送（默认值，图片较小）
        
        // 可选：设置图片格式（根据文件扩展名判断）
        QString suffix = fileInfo.suffix().toLower();
        if (suffix == "png") {
            imageElem[kTIMImageElemFormat] = 1; // PNG格式
        } else if (suffix == "jpg" || suffix == "jpeg") {
            imageElem[kTIMImageElemFormat] = 2; // JPG格式
        } else if (suffix == "bmp") {
            imageElem[kTIMImageElemFormat] = 3; // BMP格式
        } else if (suffix == "gif") {
            imageElem[kTIMImageElemFormat] = 4; // GIF格式
        }
        // 如果不设置，SDK会自动识别
        
        // 构造消息
        QJsonObject msgObj;
        QJsonArray elemArray;
        elemArray.append(imageElem);
        msgObj[kTIMMsgElemArray] = elemArray;
        msgObj[kTIMMsgSender] = userinfo.strUserId;
        qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        msgObj[kTIMMsgClientTime] = currentTime;
        msgObj[kTIMMsgServerTime] = currentTime;
        
        // 转换为JSON字符串
        QJsonDocument doc(msgObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        qDebug() << "发送图片消息，路径:" << normalizedPath;
        qDebug() << "消息JSON:" << QString::fromUtf8(jsonData);
        
        // 创建回调数据结构
        struct SendImageMsgCallbackData {
            ChatDialog* dlg;
            QString imgPath;
            QString senderName;
        };
        SendImageMsgCallbackData* callbackData = new SendImageMsgCallbackData;
        callbackData->dlg = this;
        callbackData->imgPath = imgPath;
        callbackData->senderName = userinfo.strName;
        
        // 发送消息
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgSendNewMsg(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                SendImageMsgCallbackData* data = (SendImageMsgCallbackData*)user_data;
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "发送图片消息失败，错误码:" << code << "，描述:" << errorDesc;
                    QMessageBox::critical(data->dlg, "发送失败", QString("图片消息发送失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc));
                } else {
                    qDebug() << "图片消息发送成功";
                    // 可以选择在成功后显示成功提示，或者静默处理
                }
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgSendNewMsg失败，错误码:" << ret;
            QMessageBox::critical(this, "错误", QString("调用发送接口失败，错误码: %1").arg(ret));
            delete callbackData;
        }
    }
    
    // 使用腾讯SDK发送文件消息
    void sendFileMessageViaTIMSDK(const QString& filePath, const UserInfo& userinfo, 
                                   QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        if (m_unique_group_id.isEmpty() || filePath.isEmpty()) return;
        
        // 检查文件是否存在
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            QMessageBox::warning(this, "错误", QString("文件不存在：%1").arg(filePath));
            return;
        }
        
        // 检查文件大小（腾讯SDK可能有大小限制，这里可以添加检查）
        qint64 fileSize = fileInfo.size();
        if (fileSize <= 0) {
            QMessageBox::warning(this, "错误", "文件大小为0，无法发送！");
            return;
        }
        
        // 将路径转换为本地路径格式（Windows需要使用反斜杠）
        QString normalizedPath = QDir::toNativeSeparators(filePath);
        QString fileName = fileInfo.fileName(); // 获取文件名（不含路径）
        
        // 构造文件元素
        QJsonObject fileElem;
        fileElem[kTIMElemType] = (int)kTIMElem_File;
        fileElem[kTIMFileElemFilePath] = normalizedPath; // 文件路径（必填）
        fileElem[kTIMFileElemFileName] = fileName; // 文件名（必填）
        fileElem[kTIMFileElemFileSize] = (int)fileSize; // 文件大小（必填，转换为int）
        
        // 构造消息
        QJsonObject msgObj;
        QJsonArray elemArray;
        elemArray.append(fileElem);
        msgObj[kTIMMsgElemArray] = elemArray;
        msgObj[kTIMMsgSender] = userinfo.strUserId;
        qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        msgObj[kTIMMsgClientTime] = currentTime;
        msgObj[kTIMMsgServerTime] = currentTime;
        
        // 转换为JSON字符串
        QJsonDocument doc(msgObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        qDebug() << "发送文件消息，路径:" << normalizedPath;
        qDebug() << "文件名:" << fileName << "，大小:" << fileSize << "字节";
        qDebug() << "消息JSON:" << QString::fromUtf8(jsonData);
        
        // 注册上传进度回调（全局注册一次）
        static bool s_uploadProgressCallbackRegistered = false;
        if (!s_uploadProgressCallbackRegistered) {
            TIMSetMsgElemUploadProgressCallback([](const char* json_msg, uint32_t index, uint32_t cur_size, uint32_t total_size, const void* user_data) {
                // 解析消息JSON，找到对应的文件路径
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_msg), &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    return;
                }
                
                QJsonObject msgObj = doc.object();
                if (!msgObj.contains(kTIMMsgElemArray) || !msgObj[kTIMMsgElemArray].isArray()) {
                    return;
                }
                
                QJsonArray elemArray = msgObj[kTIMMsgElemArray].toArray();
                if (index >= (uint32_t)elemArray.size()) {
                    return;
                }
                
                QJsonObject elemObj = elemArray[index].toObject();
                int elemType = elemObj[kTIMElemType].toInt();
                
                // 只处理文件元素
                if (elemType == kTIMElem_File) {
                    QString filePath = elemObj[kTIMFileElemFilePath].toString();
                    QString normalizedPath = QDir::toNativeSeparators(filePath); // 规范化路径
                    
                    qDebug() << "文件上传进度回调，路径:" << normalizedPath << "，当前大小:" << cur_size << "，总大小:" << total_size;
                    
                    // 在所有ChatDialog实例中查找匹配的进度条
                    QList<ChatDialog*>& instances = ChatDialog::getInstanceList();
                    for (ChatDialog* dlg : instances) {
                        if (!dlg) continue;
                        
                        // 尝试匹配路径（可能格式不同）
                        bool found = false;
                        QProgressBar* progressBar = nullptr;
                        QLabel* statusLabel = nullptr;
                        
                        // 首先尝试精确匹配
                        if (dlg->m_fileUploadProgressMap.contains(normalizedPath)) {
                            progressBar = dlg->m_fileUploadProgressMap[normalizedPath].first;
                            statusLabel = dlg->m_fileUploadProgressMap[normalizedPath].second;
                            found = true;
                        } else {
                            // 如果精确匹配失败，尝试匹配所有key（可能路径格式不同）
                            for (auto it = dlg->m_fileUploadProgressMap.begin(); it != dlg->m_fileUploadProgressMap.end(); ++it) {
                                QString key = it.key();
                                // 转换为相同格式后比较
                                QString normalizedKey = QDir::toNativeSeparators(key);
                                if (QFileInfo(normalizedKey).canonicalFilePath() == QFileInfo(normalizedPath).canonicalFilePath() ||
                                    normalizedKey == normalizedPath || key == filePath || normalizedKey == filePath) {
                                    progressBar = it.value().first;
                                    statusLabel = it.value().second;
                                    found = true;
                                    qDebug() << "找到匹配的文件路径，key:" << key << "，回调路径:" << normalizedPath;
                                    break;
                                }
                            }
                        }
                        
                        if (found && progressBar && total_size > 0) {
                            int progress = (int)((double)cur_size / total_size * 100);
                            qDebug() << "更新上传进度:" << progress << "% (" << cur_size << "/" << total_size << ")";
                            qDebug() << "进度条指针:" << progressBar << "，状态标签指针:" << statusLabel;
                            qDebug() << "进度条父对象:" << (progressBar ? progressBar->parent() : nullptr);
                            
                            // 直接更新（SDK回调通常在主线程中）
                            if (progressBar && progressBar->parent()) { // 检查对象是否仍然有效
                                progressBar->setValue(progress);
                                
                                // 更新状态标签显示进度
                                if (statusLabel && statusLabel->parent()) {
                                    QString sizeStr = cur_size < 1024 ? QString("%1 字节").arg(cur_size) :
                                                     cur_size < 1024 * 1024 ? QString("%1 KB").arg(cur_size / 1024.0, 0, 'f', 1) :
                                                     QString("%1 MB").arg(cur_size / (1024.0 * 1024.0), 0, 'f', 1);
                                    QString totalStr = total_size < 1024 ? QString("%1 字节").arg(total_size) :
                                                      total_size < 1024 * 1024 ? QString("%1 KB").arg(total_size / 1024.0, 0, 'f', 1) :
                                                      QString("%1 MB").arg(total_size / (1024.0 * 1024.0), 0, 'f', 1);
                                    statusLabel->setText(QString("上传中: %1 / %2 (%3%)").arg(sizeStr).arg(totalStr).arg(progress));
                                    statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                                }
                            } else {
                                qDebug() << "进度条或状态标签无效，无法更新";
                            }
                        } else {
                            qDebug() << "未找到匹配的进度条";
                            qDebug() << "回调路径:" << normalizedPath;
                            qDebug() << "map大小:" << (dlg ? dlg->m_fileUploadProgressMap.size() : 0);
                            if (dlg) {
                                qDebug() << "map中的key列表:";
                                for (auto it = dlg->m_fileUploadProgressMap.begin(); it != dlg->m_fileUploadProgressMap.end(); ++it) {
                                    qDebug() << "  key:" << it.key();
                                }
                            }
                        }
                    }
                }
            }, nullptr);
            s_uploadProgressCallbackRegistered = true;
            qDebug() << "文件上传进度回调已注册";
        }
        
        // 创建回调数据结构
        struct SendFileMsgCallbackData {
            ChatDialog* dlg;
            QString filePath;
            QString senderName;
            QProgressBar* progressBar;
            QLabel* statusLabel;
            qint64 totalSize;
        };
        SendFileMsgCallbackData* callbackData = new SendFileMsgCallbackData;
        callbackData->dlg = this;
        callbackData->filePath = filePath;
        callbackData->senderName = userinfo.strName;
        callbackData->progressBar = progressBar;
        callbackData->statusLabel = statusLabel;
        callbackData->totalSize = fileSize;
        
        // 更新状态为"正在上传"
        if (statusLabel) {
            statusLabel->setText("正在上传...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            progressBar->setValue(0);
        }
        
        // 保存进度条映射（用于进度回调更新）
        // 使用规范化路径作为key，确保路径格式一致
        QString normalizedKey = QDir::toNativeSeparators(filePath);
        m_fileUploadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        qDebug() << "保存文件上传进度映射，路径:" << normalizedKey << "，进度条:" << progressBar << "，状态标签:" << statusLabel;
        
        // 发送消息
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgSendNewMsg(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                SendFileMsgCallbackData* data = (SendFileMsgCallbackData*)user_data;
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "发送文件消息失败，错误码:" << code << "，描述:" << errorDesc;
                    
                    // 从map中获取进度条和状态标签
                    QString normalizedKey = QDir::toNativeSeparators(data->filePath);
                    QProgressBar* progressBar = nullptr;
                    QLabel* statusLabel = nullptr;
                    
                    if (data->dlg->m_fileUploadProgressMap.contains(normalizedKey)) {
                        progressBar = data->dlg->m_fileUploadProgressMap[normalizedKey].first;
                        statusLabel = data->dlg->m_fileUploadProgressMap[normalizedKey].second;
                    }
                    
                    // 更新状态为失败
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText("发送失败");
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                    if (progressBar && progressBar->parent()) {
                        progressBar->setValue(0);
                    }
                    
                    QMessageBox::critical(data->dlg, "发送失败", QString("文件消息发送失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc));
                    
                    // 从进度映射中移除
                    data->dlg->m_fileUploadProgressMap.remove(normalizedKey);
                } else {
                    qDebug() << "文件消息发送成功";
                    
                    // 从map中获取进度条和状态标签（确保使用正确的路径key）
                    QString normalizedKey = QDir::toNativeSeparators(data->filePath);
                    QProgressBar* progressBar = nullptr;
                    QLabel* statusLabel = nullptr;
                    
                    if (data->dlg->m_fileUploadProgressMap.contains(normalizedKey)) {
                        progressBar = data->dlg->m_fileUploadProgressMap[normalizedKey].first;
                        statusLabel = data->dlg->m_fileUploadProgressMap[normalizedKey].second;
                    }
                    
                    // 更新状态为成功
                    if (statusLabel && statusLabel->parent()) { // 检查对象是否仍然有效
                        statusLabel->setText("发送成功");
                        statusLabel->setStyleSheet("color: green; font-size: 10px;");
                    }
                    if (progressBar && progressBar->parent()) { // 检查对象是否仍然有效
                        progressBar->setValue(100);
                        // 3秒后隐藏进度条
                        QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                            if (progressBar && progressBar->parent()) {
                                progressBar->hide();
                            }
                            if (statusLabel && statusLabel->parent()) {
                                statusLabel->hide();
                            }
                        });
                    }
                    
                    // 从进度映射中移除
                    data->dlg->m_fileUploadProgressMap.remove(normalizedKey);
                }
                
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgSendNewMsg失败，错误码:" << ret;
            QString errorDesc = QString("错误码: %1").arg(ret);
            QMessageBox::critical(this, "发送失败", QString("文件消息发送失败\n%1").arg(errorDesc));
            delete callbackData;
        }
    }
    
    // 接收新消息的回调处理
    void onRecvNewMsg(const char* json_msg_array)
    {
        // 解析JSON消息数组
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_msg_array), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "解析消息JSON失败:" << parseError.errorString();
            return;
        }
        
        if (!doc.isArray()) {
            qDebug() << "消息JSON不是数组格式";
            return;
        }
        
        QJsonArray msgArray = doc.array();
        UserInfo userInfo = CommonInfo::GetData();
        
        for (const QJsonValue& value : msgArray) {
            if (!value.isObject()) continue;
            
            QJsonObject msgObj = value.toObject();
            
            // 检查是否是当前群组的消息
            QString convId = msgObj[kTIMMsgConvId].toString();
            int convType = msgObj[kTIMMsgConvType].toInt();
            
            if (convType != kTIMConv_Group || convId != m_unique_group_id) {
                continue; // 不是当前群组的消息，跳过
            }
            
            // 检查是否是自己的消息（自己发送的消息已经在发送时显示，避免重复）
            bool isFromSelf = msgObj[kTIMMsgIsFormSelf].toBool();
            QString senderId = msgObj[kTIMMsgSender].toString();
            
            // 解析消息元素
            if (!msgObj.contains(kTIMMsgElemArray) || !msgObj[kTIMMsgElemArray].isArray()) {
                continue;
            }
            
            QJsonArray elemArray = msgObj[kTIMMsgElemArray].toArray();
            for (const QJsonValue& elemValue : elemArray) {
                if (!elemValue.isObject()) continue;
                
                QJsonObject elemObj = elemValue.toObject();
                int elemType = elemObj[kTIMElemType].toInt();
                
                QString senderName = senderId;
                // 尝试从发送者信息中获取名称
                if (msgObj.contains(kTIMMsgSenderProfile)) {
                    QJsonObject senderProfile = msgObj[kTIMMsgSenderProfile].toObject();
                    if (senderProfile.contains("user_profile_nick_name")) {
                        senderName = senderProfile["user_profile_nick_name"].toString();
                    }
                }
                
                // 如果是自己的消息，已经显示过了，跳过
                if (isFromSelf || senderId == userInfo.teacher_unique_id) {
                    continue;
                }
                
                // 根据元素类型处理
                if (elemType == kTIMElem_Text) {
                    QString text = elemObj[kTIMTextElemContent].toString();
                    addTextMessage(":/res/img/home.png", senderName, text, false);
                } else if (elemType == kTIMElem_Image) {
                    // 处理图片消息
                    handleImageMessage(elemObj, senderName, false);
                } else if (elemType == kTIMElem_File) {
                    // 处理文件消息
                    handleFileMessage(elemObj, senderName, false);
                } else if (elemType == kTIMElem_Sound) {
                    // TODO: 处理语音消息
                    addTextMessage(":/res/img/home.png", senderName, "[语音]", false);
                }
            }
        }
    }
    
    // 加载历史消息（包括离线消息）
    void loadHistoryMessages()
    {
        if (m_unique_group_id.isEmpty()) return;
        
        // 构造获取消息参数
        QJsonObject getMsgParam;
        // 不指定 LastMsg，表示获取最新的消息
        getMsgParam[kTIMMsgGetMsgListParamCount] = 100; // 获取最近100条消息
        getMsgParam[kTIMMsgGetMsgListParamIsRamble] = true; // 开启漫游消息，会从云端拉取离线消息
        getMsgParam[kTIMMsgGetMsgListParamIsForward] = false; // false表示获取比最新消息更旧的消息（历史消息）
        
        QJsonDocument doc(getMsgParam);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 创建回调数据结构
        struct LoadHistoryCallbackData {
            ChatDialog* dlg;
            QString groupId;
        };
        LoadHistoryCallbackData* callbackData = new LoadHistoryCallbackData;
        callbackData->dlg = this;
        callbackData->groupId = m_unique_group_id;
        
        // 调用获取消息列表接口
        QByteArray groupIdBytes = m_unique_group_id.toUtf8();
        int ret = TIMMsgGetMsgList(groupIdBytes.constData(), kTIMConv_Group, jsonData.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                LoadHistoryCallbackData* data = (LoadHistoryCallbackData*)user_data;
                ChatDialog* dlg = data->dlg;
                
                if (code != TIM_SUCC) {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "加载历史消息失败，群组ID:" << data->groupId << "，错误码:" << code << "，描述:" << errorDesc;
                    delete data;
                    return;
                }
                
                // 解析返回的消息列表
                if (json_params) {
                    QJsonParseError parseError;
                    QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_params), &parseError);
                    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                        QJsonArray msgArray = doc.array();
                        qDebug() << "加载历史消息成功，群组ID:" << data->groupId << "，消息数量:" << msgArray.size();
                        
                        // 将历史消息添加到界面（按时间倒序，因为获取的是从新到旧）
                        // 需要反转顺序，从旧到新显示
                        QVector<QJsonObject> msgList;
                        for (const QJsonValue& value : msgArray) {
                            if (value.isObject()) {
                                msgList.prepend(value.toObject()); // 使用prepend保持从旧到新的顺序
                            }
                        }
                        
                        // 批量显示历史消息
                        UserInfo userInfo = CommonInfo::GetData();
                        for (const QJsonObject& msgObj : msgList) {
                            QString convId = msgObj[kTIMMsgConvId].toString();
                            if (convId != data->groupId) continue; // 只处理当前群组的消息
                            
                            bool isFromSelf = msgObj[kTIMMsgIsFormSelf].toBool();
                            QString senderId = msgObj[kTIMMsgSender].toString();
                            
                            if (!msgObj.contains(kTIMMsgElemArray) || !msgObj[kTIMMsgElemArray].isArray()) {
                                continue;
                            }
                            
                            QJsonArray elemArray = msgObj[kTIMMsgElemArray].toArray();
                            for (const QJsonValue& elemValue : elemArray) {
                                if (!elemValue.isObject()) continue;
                                
                                QJsonObject elemObj = elemValue.toObject();
                                int elemType = elemObj[kTIMElemType].toInt();
                                
                                QString senderName = senderId;
                                if (msgObj.contains(kTIMMsgSenderProfile)) {
                                    QJsonObject senderProfile = msgObj[kTIMMsgSenderProfile].toObject();
                                    if (senderProfile.contains("user_profile_nick_name")) {
                                        senderName = senderProfile["user_profile_nick_name"].toString();
                                    }
                                }
                                
                                bool isMine = (isFromSelf || senderId == userInfo.teacher_unique_id);
                                
                                if (elemType == kTIMElem_Text) {
                                    QString text = elemObj[kTIMTextElemContent].toString();
                                    dlg->addTextMessage(":/res/img/home.png", senderName, text, isMine);
                                } else if (elemType == kTIMElem_Image) {
                                    // 处理历史消息中的图片
                                    dlg->handleImageMessage(elemObj, senderName, isMine);
                                } else if (elemType == kTIMElem_File) {
                                    // 处理历史消息中的文件
                                    dlg->handleFileMessage(elemObj, senderName, isMine);
                                } else if (elemType == kTIMElem_Sound) {
                                    dlg->addTextMessage(":/res/img/home.png", senderName, "[语音]", isMine);
                                }
                            }
                        }
                    } else {
                        qDebug() << "解析历史消息JSON失败:" << parseError.errorString();
                    }
                }
                
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgGetMsgList失败，错误码:" << ret;
            delete callbackData;
        }
    }

    // 静态实例列表和回调注册标志（用于消息分发）
    static QList<ChatDialog*>& getInstanceList()
    {
        static QList<ChatDialog*> s_instances;
        return s_instances;
    }
    
    // 静态函数：接收新消息回调（用于分发消息到所有实例）
    static void staticRecvNewMsgCallback(const char* json_msg_array, const void* user_data)
    {
        // 分发消息到所有实例
        QList<ChatDialog*>& instances = getInstanceList();
        for (ChatDialog* dlg : instances) {
            if (dlg) {
                dlg->onRecvNewMsg(json_msg_array);
            }
        }
    }
    
    // 注册/注销实例到静态列表（用于消息分发）
    void registerInstance()
    {
        // 确保回调已注册（如果还没注册的话）
        ensureCallbackRegistered();
        
        QList<ChatDialog*>& instances = getInstanceList();
        
        if (!instances.contains(this)) {
            instances.append(this);
        }
    }
    
    void unregisterInstance()
    {
        QList<ChatDialog*>& instances = getInstanceList();
        instances.removeAll(this);
    }

    // 处理接收到的图片消息
    void handleImageMessage(const QJsonObject& imageElem, const QString& senderName, bool isMine)
    {
        // 获取图片URL（优先使用大图，PC端建议使用大图）
        QString imageUrl = imageElem[kTIMImageElemLargeUrl].toString(); // 大图URL
        QString imageId = imageElem[kTIMImageElemLargeId].toString();   // 大图ID
        
        // 如果大图URL为空，尝试使用缩略图
        if (imageUrl.isEmpty()) {
            imageUrl = imageElem[kTIMImageElemThumbUrl].toString();
            imageId = imageElem[kTIMImageElemThumbId].toString();
        }
        
        // 如果还是为空，尝试使用原图
        if (imageUrl.isEmpty()) {
            imageUrl = imageElem[kTIMImageElemOrigUrl].toString();
            imageId = imageElem[kTIMImageElemOrigId].toString();
        }
        
        if (imageUrl.isEmpty() && imageId.isEmpty()) {
            qDebug() << "图片消息URL和ID都为空，无法下载";
            addTextMessage(":/res/img/home.png", senderName, "[图片]（无法获取）", isMine);
            return;
        }
        
        // 创建临时目录保存下载的图片
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir dir;
        if (!dir.exists(cacheDir)) {
            dir.mkpath(cacheDir);
        }
        
        // 生成本地保存路径
        QString fileName = QString("image_%1_%2.jpg").arg(imageId.isEmpty() ? "unknown" : imageId).arg(QDateTime::currentDateTime().toSecsSinceEpoch());
        QString localPath = QDir(cacheDir).filePath(fileName);
        
        // 如果URL不为空，直接使用HTTP下载（SDK回调不可靠，优先使用HTTP）
        if (!imageUrl.isEmpty()) {
            downloadImageFromUrl(imageUrl, localPath, senderName, isMine);
        } else if (!imageId.isEmpty()) {
            // 只有ID没有URL，尝试使用SDK下载
            downloadImageFromSDK(imageElem, localPath, senderName, isMine);
        } else {
            qDebug() << "图片消息URL和ID都为空，无法下载";
            addTextMessage(":/res/img/home.png", senderName, "[图片]（无法获取）", isMine);
        }
    }
    
    // 处理接收到的文件消息
    void handleFileMessage(const QJsonObject& fileElem, const QString& senderName, bool isMine)
    {
        // 获取文件信息
        QString fileName = fileElem[kTIMFileElemFileName].toString();
        int fileSize = fileElem[kTIMFileElemFileSize].toInt();
        QString fileUrl = fileElem[kTIMFileElemUrl].toString();
        QString fileId = fileElem[kTIMFileElemFileId].toString();
        
        if (fileName.isEmpty() && fileUrl.isEmpty() && fileId.isEmpty()) {
            qDebug() << "文件消息缺少必要信息，无法处理";
            addTextMessage(":/res/img/home.png", senderName, "[文件]（信息不完整）", isMine);
            return;
        }
        
        // 创建临时目录保存下载的文件
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir dir;
        if (!dir.exists(cacheDir)) {
            dir.mkpath(cacheDir);
        }
        
        // 生成本地保存路径（使用原始文件名）
        QString localFileName = fileName.isEmpty() ? QString("file_%1").arg(fileId.isEmpty() ? QString::number(QDateTime::currentMSecsSinceEpoch()) : fileId) : fileName;
        QString localPath = QDir(cacheDir).filePath(localFileName);
        
        // 先创建文件消息UI（带进度条和状态标签）
        QPair<QProgressBar*, QLabel*> progressWidgets = addFileMessageWithDownloadProgress(":/res/img/home.png", senderName, localPath, fileName, fileSize, isMine);
        
        // 保存文件下载信息，用于重试
        QString normalizedKey = QDir::toNativeSeparators(localPath);
        FileDownloadInfo downloadInfo;
        downloadInfo.fileUrl = fileUrl;
        downloadInfo.fileElem = fileElem;
        downloadInfo.savePath = localPath;
        downloadInfo.senderName = senderName;
        downloadInfo.fileName = fileName;
        downloadInfo.fileSize = fileSize;
        downloadInfo.isMine = isMine;
        downloadInfo.progressBar = progressWidgets.first;
        downloadInfo.statusLabel = progressWidgets.second;
        m_fileDownloadInfoMap[normalizedKey] = downloadInfo;
        
        // 如果文件URL不为空，使用HTTP下载
        if (!fileUrl.isEmpty()) {
            // 下载文件（使用HTTP下载，带进度显示）
            downloadFileFromUrl(fileUrl, localPath, senderName, fileName, fileSize, isMine, progressWidgets.first, progressWidgets.second);
        } else if (!fileId.isEmpty()) {
            // 如果只有fileId没有URL，尝试使用腾讯SDK下载
            downloadFileFromSDK(fileElem, localPath, senderName, fileName, fileSize, isMine, progressWidgets.first, progressWidgets.second);
        } else {
            // 如果既没有URL也没有fileId，显示文件信息（可能文件还在上传中）
            if (progressWidgets.second) {
                progressWidgets.second->setText("等待文件上传完成...");
                progressWidgets.second->setStyleSheet("color: orange; font-size: 10px;");
            }
            if (progressWidgets.first) {
                progressWidgets.first->setValue(0);
            }
        }
    }
    
    // 添加带下载进度条的文件消息（用于接收文件）
    QPair<QProgressBar*, QLabel*> addFileMessageWithDownloadProgress(const QString& avatarPath, const QString& senderName,
                                                                     const QString& filePath, const QString& fileName,
                                                                     int fileSize, bool isMine)
    {
        QDateTime now = QDateTime::currentDateTime();
        if (!m_hasLastMessage || m_lastMessage.time.secsTo(now) > 180) addTimeLabel(now);
        bool hideAvatar = shouldHideAvatar(senderName, isMine, now);

        QString displayFileName = fileName.isEmpty() ? QFileInfo(filePath).fileName() : fileName;
        QString fileSizeStr = fileSize < 1024 ? QString("%1 字节").arg(fileSize) :
                             fileSize < 1024 * 1024 ? QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 1) :
                             QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 1);

        // 创建文件消息容器
        QWidget* fileWidget = new QWidget();
        QVBoxLayout* fileLayout = new QVBoxLayout(fileWidget);
        fileLayout->setContentsMargins(5, 5, 5, 5);
        fileLayout->setSpacing(5);
        
        // 文件名和大小标签
        ClickableLabelEx* lblFile = new ClickableLabelEx(fileWidget);
        lblFile->setText(displayFileName + "\n" + fileSizeStr);
        lblFile->setStyleSheet("qproperty-alignment: AlignCenter;");
        lblFile->setMinimumSize(120, 50);
        connect(lblFile, &ClickableLabelEx::clicked, this, [=]() {
            QFileInfo fi(filePath);
            if (fi.exists()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
            } else {
                QMessageBox::information(this, "提示", "文件尚未下载完成");
            }
        });
        fileLayout->addWidget(lblFile);
        
        // 创建下载进度条（设置父对象为fileWidget，确保生命周期）
        QProgressBar* progressBar = new QProgressBar(fileWidget);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        progressBar->setTextVisible(true);
        progressBar->setFormat("%p%");
        fileLayout->addWidget(progressBar);
        
        // 创建状态标签（设置父对象为fileWidget，确保生命周期）
        QLabel* statusLabel = new QLabel(fileWidget);
        statusLabel->setText("准备下载...");
        statusLabel->setStyleSheet("color: gray; font-size: 10px;");
        fileLayout->addWidget(statusLabel);
        
        // 创建重试按钮（初始隐藏，下载失败时显示）
        QPushButton* retryButton = new QPushButton("重试", fileWidget);
        retryButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 4px 8px; font-size: 10px; border: none; border-radius: 3px;");
        retryButton->setMaximumWidth(60);
        retryButton->hide(); // 初始隐藏
        fileLayout->addWidget(retryButton);

        QWidget* msgWidget = buildMessageWidget(avatarPath, senderName, fileWidget, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, "[文件] " + displayFileName, isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
        
        // 返回进度条、状态标签和重试按钮的指针
        // 使用QVBoxLayout存储重试按钮，以便后续访问
        return qMakePair(progressBar, statusLabel);
    }
    
    // 连接重试按钮（辅助函数，避免在lambda中使用QObject::connect）
    void connectRetryButton(QPushButton* btn, const QString& normalizedKey)
    {
        if (!btn || !m_fileDownloadInfoMap.contains(normalizedKey)) {
            return;
        }
        // 断开之前的连接
        btn->disconnect();
        // 连接重试按钮的点击事件
        QObject::connect(btn, &QPushButton::clicked, this, [this, normalizedKey]() {
            retryFileDownload(normalizedKey);
        });
    }
    
    // 重试文件下载
    void retryFileDownload(const QString& normalizedKey)
    {
        if (!m_fileDownloadInfoMap.contains(normalizedKey)) {
            qDebug() << "找不到文件下载信息，无法重试";
            return;
        }
        
        FileDownloadInfo& info = m_fileDownloadInfoMap[normalizedKey];
        
        qDebug() << "重试下载文件:" << info.fileName << "，路径:" << info.savePath;
        
        // 重置进度条和状态标签
        if (info.progressBar && info.progressBar->parent()) {
            info.progressBar->setValue(0);
            info.progressBar->show();
        }
        if (info.statusLabel && info.statusLabel->parent()) {
            info.statusLabel->setText("正在重试...");
            info.statusLabel->setStyleSheet("color: blue; font-size: 10px;");
            info.statusLabel->show();
        }
        
        // 隐藏重试按钮
        if (info.statusLabel && info.statusLabel->parent()) {
            QWidget* parentWidget = qobject_cast<QWidget*>(info.statusLabel->parent());
            if (parentWidget) {
                QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(parentWidget->layout());
                if (layout) {
                    for (int i = 0; i < layout->count(); i++) {
                        QPushButton* btn = qobject_cast<QPushButton*>(layout->itemAt(i)->widget());
                        if (btn && btn->text() == "重试") {
                            btn->hide();
                            break;
                        }
                    }
                }
            }
        }
        
        // 重新开始下载
        if (!info.fileUrl.isEmpty()) {
            downloadFileFromUrl(info.fileUrl, info.savePath, info.senderName, info.fileName, 
                              info.fileSize, info.isMine, info.progressBar, info.statusLabel);
        } else if (!info.fileElem.isEmpty()) {
            downloadFileFromSDK(info.fileElem, info.savePath, info.senderName, info.fileName,
                              info.fileSize, info.isMine, info.progressBar, info.statusLabel);
        }
    }
    
    // 从URL下载文件（带进度显示）
    void downloadFileFromUrl(const QString& fileUrl, const QString& savePath, const QString& senderName, 
                             const QString& fileName, int fileSize, bool isMine,
                             QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        QUrl url(fileUrl);
        if (!url.isValid()) {
            qDebug() << "文件URL无效:" << fileUrl;
            if (statusLabel) {
                statusLabel->setText("URL无效");
                statusLabel->setStyleSheet("color: red; font-size: 10px;");
            }
            return;
        }
        
        // 检查SSL支持（如果系统不支持SSL，尝试将HTTPS改为HTTP或继续下载）
        if (url.scheme().toLower() == "https") {
            if (!QSslSocket::supportsSsl()) {
                qDebug() << "系统不支持SSL，尝试将HTTPS改为HTTP或使用其他下载方式";
                // 尝试将HTTPS改为HTTP（可能不工作，但试试）
                QString httpUrl = fileUrl;
                httpUrl.replace("https://", "http://");
                if (httpUrl != fileUrl) {
                    qDebug() << "尝试使用HTTP替代HTTPS:" << httpUrl;
                    url = QUrl(httpUrl);
                    if (!url.isValid()) {
                        qDebug() << "HTTP URL无效，继续使用HTTPS并忽略SSL错误";
                        url = QUrl(fileUrl); // 恢复原始URL
                    }
                } else {
                    qDebug() << "无法将HTTPS改为HTTP，继续使用HTTPS并忽略SSL错误";
                }
            }
        }
        
        // 检查文件是否已部分下载（断点续传）
        qint64 existingFileSize = 0;
        bool isResume = false;
        QFileInfo fileInfo(savePath);
        if (fileInfo.exists()) {
            existingFileSize = fileInfo.size();
            if (existingFileSize > 0 && existingFileSize < fileSize) {
                isResume = true;
                qDebug() << "检测到已部分下载的文件，大小:" << existingFileSize << "字节，将断点续传";
            } else if (existingFileSize >= fileSize) {
                // 文件已完整下载
                qDebug() << "文件已完整下载，大小:" << existingFileSize << "字节";
                if (statusLabel && statusLabel->parent()) {
                    statusLabel->setText("文件已存在");
                    statusLabel->setStyleSheet("color: green; font-size: 10px;");
                }
                if (progressBar && progressBar->parent()) {
                    progressBar->setValue(100);
                }
                return;
            }
        }
        
        // 更新状态为"正在下载"或"正在续传"
        if (statusLabel) {
            statusLabel->setText(isResume ? "正在续传..." : "正在下载...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            if (isResume && fileSize > 0) {
                // 设置初始进度（已下载部分）
                int initialProgress = (int)((double)existingFileSize / fileSize * 100);
                progressBar->setValue(initialProgress);
            } else {
                progressBar->setValue(0);
            }
        }
        
        // 保存下载进度映射
        QString normalizedKey = QDir::toNativeSeparators(savePath);
        m_fileDownloadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        qDebug() << "保存文件下载进度映射，路径:" << normalizedKey << "，断点续传:" << isResume;
        
        // 保存this指针，以便在lambda中使用
        ChatDialog* dlg = this;
        
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest networkRequest;
        networkRequest.setUrl(url);
        
        // 如果是断点续传，设置 Range 请求头
        if (isResume) {
            QString rangeHeader = QString("bytes=%1-").arg(existingFileSize);
            networkRequest.setRawHeader("Range", rangeHeader.toUtf8());
            qDebug() << "设置 Range 请求头:" << rangeHeader;
        }
        
        // 如果是HTTPS，设置SSL配置
        if (url.scheme().toLower() == "https") {
            QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
            networkRequest.setSslConfiguration(sslConfig);
        }
        
        QNetworkReply* reply = manager->get(networkRequest);
        
        // 忽略SSL错误
        connect(reply, &QNetworkReply::sslErrors, this, [=](const QList<QSslError>& errors) {
            qDebug() << "SSL错误，忽略证书验证，URL:" << fileUrl;
            reply->ignoreSslErrors();
        });
        
        // 保存断点续传信息到lambda中
        qint64 resumeOffset = existingFileSize;
        bool isResumeDownload = isResume;
        
        // 连接下载进度信号（考虑断点续传）
        connect(reply, &QNetworkReply::downloadProgress, this, [progressBar, statusLabel, fileSize, resumeOffset, isResumeDownload](qint64 bytesReceived, qint64 bytesTotal) {
            if (progressBar && progressBar->parent()) {
                // 计算总进度（已下载部分 + 本次下载部分）
                qint64 totalReceived = isResumeDownload ? resumeOffset + bytesReceived : bytesReceived;
                qint64 totalSize = fileSize > 0 ? fileSize : (isResumeDownload ? resumeOffset + bytesTotal : bytesTotal);
                
                if (totalSize > 0) {
                    int progress = (int)((double)totalReceived / totalSize * 100);
                    progressBar->setValue(progress);
                    
                    if (statusLabel && statusLabel->parent()) {
                        QString receivedStr = totalReceived < 1024 ? QString("%1 字节").arg(totalReceived) :
                                             totalReceived < 1024 * 1024 ? QString("%1 KB").arg(totalReceived / 1024.0, 0, 'f', 1) :
                                             QString("%1 MB").arg(totalReceived / (1024.0 * 1024.0), 0, 'f', 1);
                        QString totalStr = totalSize < 1024 ? QString("%1 字节").arg(totalSize) :
                                          totalSize < 1024 * 1024 ? QString("%1 KB").arg(totalSize / 1024.0, 0, 'f', 1) :
                                          QString("%1 MB").arg(totalSize / (1024.0 * 1024.0), 0, 'f', 1);
                        QString statusText = isResumeDownload ? 
                            QString("续传中: %1 / %2 (%3%)").arg(receivedStr).arg(totalStr).arg(progress) :
                            QString("下载中: %1 / %2 (%3%)").arg(receivedStr).arg(totalStr).arg(progress);
                        statusLabel->setText(statusText);
                        statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                    }
                }
            }
        });
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, manager, fileUrl, dlg, savePath, fileSize, resumeOffset, isResumeDownload]() {
            int error = reply->error();
            QString normalizedKey = QDir::toNativeSeparators(savePath);
            QProgressBar* progressBar = nullptr;
            QLabel* statusLabel = nullptr;

            if (m_fileDownloadProgressMap.contains(normalizedKey)) {
                progressBar = m_fileDownloadProgressMap[normalizedKey].first;
                statusLabel = m_fileDownloadProgressMap[normalizedKey].second;
            }

            // 检查HTTP状态码（206表示部分内容，支持断点续传）
            int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            bool isPartialContent = (httpStatusCode == 206); // HTTP 206 Partial Content
            
            if (reply->error() == QNetworkReply::NoError || isPartialContent) {
                QByteArray fileData = reply->readAll();
                if (fileData.isEmpty() && !isResumeDownload) {
                    qDebug() << "文件数据为空";
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText("文件数据为空");
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                }
                else {
                    // 断点续传时使用追加模式，否则使用覆盖模式
                    QFile file(savePath);
                    QIODevice::OpenMode openMode = isResumeDownload ? QIODevice::Append : QIODevice::WriteOnly;
                    
                    if (file.open(openMode)) {
                        file.write(fileData);
                        file.close();
                        
                        // 获取最终文件大小
                        QFileInfo finalFileInfo(savePath);
                        qint64 finalSize = finalFileInfo.exists() ? finalFileInfo.size() : 0;
                        qDebug() << "文件下载成功:" << savePath;
                        qDebug() << "本次下载:" << fileData.size() << "字节";
                        if (isResumeDownload) {
                            qDebug() << "断点续传: 原有" << resumeOffset << "字节 + 本次" << fileData.size() << "字节 = 总计" << finalSize << "字节";
                        }

                        // 更新状态为成功
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("下载成功");
                            statusLabel->setStyleSheet("color: green; font-size: 10px;");
                        }
                        if (progressBar && progressBar->parent()) {
                            progressBar->setValue(100);
                            // 3秒后隐藏进度条
                            QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                                if (progressBar && progressBar->parent()) {
                                    progressBar->hide();
                                }
                                if (statusLabel && statusLabel->parent()) {
                                    statusLabel->hide();
                                }
                                });
                        }
                    }
                    else {
                        qDebug() << "保存文件失败:" << savePath;
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("保存失败");
                            statusLabel->setStyleSheet("color: red; font-size: 10px;");
                        }
                    }
                }
            }
            else {
                QString qerror = reply->errorString();
                qDebug() << "文件下载失败，URL:" << fileUrl;
                qDebug() << "错误码:" << error << "，错误描述:" << qerror;
                if (statusLabel && statusLabel->parent()) {
                    statusLabel->setText(QString("下载失败: %1").arg(qerror));
                    statusLabel->setStyleSheet("color: red; font-size: 10px;");
                }

                // 下载失败时，显示重试按钮
                if (dlg && dlg->m_fileDownloadInfoMap.contains(normalizedKey)) {
                    FileDownloadInfo& info = dlg->m_fileDownloadInfoMap[normalizedKey];
                    // 查找重试按钮（在状态标签的父布局中）
                    if (statusLabel && statusLabel->parent()) {
                        QWidget* parentWidget = qobject_cast<QWidget*>(statusLabel->parent());
                        if (parentWidget) {
                            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(parentWidget->layout());
                            if (layout) {
                                // 查找重试按钮
                                for (int i = 0; i < layout->count(); i++) {
                                    QPushButton* btn = qobject_cast<QPushButton*>(layout->itemAt(i)->widget());
                                    if (btn && btn->text() == "重试") {
                                        btn->show();
                                        // 使用QTimer::singleShot延迟连接，避免在回调lambda中直接调用QObject::connect
                                        QString retryKey = normalizedKey;
                                        QTimer::singleShot(0, dlg, [dlg, btn, retryKey]() {
                                            dlg->connectRetryButton(btn, retryKey);
                                        });
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // 从下载进度映射中移除（但保留下载信息映射，以便重试）
                m_fileDownloadProgressMap.remove(normalizedKey);

                reply->deleteLater();
                manager->deleteLater();
            }
        });
    }
    
    // 使用腾讯SDK下载文件（如果只有fileId没有URL）
    void downloadFileFromSDK(const QJsonObject& fileElem, const QString& savePath, const QString& senderName,
                             const QString& fileName, int fileSize, bool isMine,
                             QProgressBar* progressBar = nullptr, QLabel* statusLabel = nullptr)
    {
        qDebug() << "使用腾讯SDK下载文件，fileId:" << fileElem[kTIMFileElemFileId].toString();
        
        // 更新状态为"正在下载"
        if (statusLabel) {
            statusLabel->setText("正在下载（SDK）...");
            statusLabel->setStyleSheet("color: blue; font-size: 10px;");
        }
        if (progressBar) {
            progressBar->setValue(0);
        }
        
        // 保存下载进度映射
        QString normalizedKey = QDir::toNativeSeparators(savePath);
        m_fileDownloadProgressMap[normalizedKey] = qMakePair(progressBar, statusLabel);
        
        // 构造文件元素JSON（用于下载）
        QJsonDocument elemDoc(fileElem);
        QByteArray elemJsonData = elemDoc.toJson(QJsonDocument::Compact);
        QByteArray pathBytes = savePath.toUtf8();
        
        // 创建回调数据结构
        struct DownloadFileCallbackData {
            ChatDialog* dlg;
            QString savePath;
            QString senderName;
            QString fileName;
            QProgressBar* progressBar;
            QLabel* statusLabel;
        };
        DownloadFileCallbackData* callbackData = new DownloadFileCallbackData;
        callbackData->dlg = this;
        callbackData->savePath = savePath;
        callbackData->senderName = senderName;
        callbackData->fileName = fileName;
        callbackData->progressBar = progressBar;
        callbackData->statusLabel = statusLabel;
        
        // 调用腾讯SDK下载接口
        int ret = TIMMsgDownloadElemToPath(elemJsonData.constData(), pathBytes.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                DownloadFileCallbackData* data = (DownloadFileCallbackData*)user_data;
                ChatDialog* dlg = data->dlg;
                
                QString normalizedKey = QDir::toNativeSeparators(data->savePath);
                QProgressBar* progressBar = nullptr;
                QLabel* statusLabel = nullptr;
                
                if (dlg->m_fileDownloadProgressMap.contains(normalizedKey)) {
                    progressBar = dlg->m_fileDownloadProgressMap[normalizedKey].first;
                    statusLabel = dlg->m_fileDownloadProgressMap[normalizedKey].second;
                }
                
                if (code == TIM_SUCC) {
                    // 下载成功，检查文件是否存在
                    QFile file(data->savePath);
                    if (file.exists()) {
                        qDebug() << "SDK文件下载成功:" << data->savePath;
                        
                        // 更新状态为成功
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("下载成功");
                            statusLabel->setStyleSheet("color: green; font-size: 10px;");
                        }
                        if (progressBar && progressBar->parent()) {
                            progressBar->setValue(100);
                            // 3秒后隐藏进度条
                            QTimer::singleShot(3000, progressBar, [progressBar, statusLabel]() {
                                if (progressBar && progressBar->parent()) {
                                    progressBar->hide();
                                }
                                if (statusLabel && statusLabel->parent()) {
                                    statusLabel->hide();
                                }
                            });
                        }
                    } else {
                        qDebug() << "SDK下载成功但文件不存在:" << data->savePath;
                        if (statusLabel && statusLabel->parent()) {
                            statusLabel->setText("文件不存在");
                            statusLabel->setStyleSheet("color: red; font-size: 10px;");
                        }
                    }
                } else {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    qDebug() << "SDK文件下载失败，错误码:" << code << "，描述:" << errorDesc;
                    if (statusLabel && statusLabel->parent()) {
                        statusLabel->setText(QString("下载失败: %1").arg(errorDesc));
                        statusLabel->setStyleSheet("color: red; font-size: 10px;");
                    }
                    
                    // SDK下载失败时，也显示重试按钮
                    if (dlg && dlg->m_fileDownloadInfoMap.contains(normalizedKey)) {
                        FileDownloadInfo& info = dlg->m_fileDownloadInfoMap[normalizedKey];
                        if (statusLabel && statusLabel->parent()) {
                            QWidget* parentWidget = qobject_cast<QWidget*>(statusLabel->parent());
                            if (parentWidget) {
                                QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(parentWidget->layout());
                                if (layout) {
                                    for (int i = 0; i < layout->count(); i++) {
                                        QPushButton* btn = qobject_cast<QPushButton*>(layout->itemAt(i)->widget());
                                        if (btn && btn->text() == "重试") {
                                            btn->show();
                                            // 使用QTimer::singleShot延迟连接，避免在回调lambda中直接调用QObject::connect
                                            QString retryKey = normalizedKey;
                                            QTimer::singleShot(0, dlg, [dlg, btn, retryKey]() {
                                                dlg->connectRetryButton(btn, retryKey);
                                            });
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // 从下载进度映射中移除（但保留下载信息映射，以便重试）
                dlg->m_fileDownloadProgressMap.remove(normalizedKey);
                
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMMsgDownloadElemToPath失败，错误码:" << ret;
            if (statusLabel && statusLabel->parent()) {
                statusLabel->setText("下载初始化失败");
                statusLabel->setStyleSheet("color: red; font-size: 10px;");
            }
            m_fileDownloadProgressMap.remove(normalizedKey);
            delete callbackData;
        }
    }
    
    // 从URL下载图片（使用HTTP/HTTPS）
    void downloadImageFromUrl(const QString& imageUrl, const QString& savePath, const QString& senderName, bool isMine)
    {
        QUrl url(imageUrl);
        if (!url.isValid()) {
            qDebug() << "图片URL无效:" << imageUrl;
            addTextMessage(":/res/img/home.png", senderName, "[图片]（URL无效）", isMine);
            return;
        }
        
        // 检查SSL支持
        if (url.scheme().toLower() == "https") {
            if (!QSslSocket::supportsSsl()) {
                qDebug() << "系统不支持SSL，尝试将HTTPS改为HTTP或使用其他下载方式";
                // 尝试将HTTPS改为HTTP（可能不工作，但试试）
                QString httpUrl = imageUrl;
                httpUrl.replace("https://", "http://");
                if (httpUrl != imageUrl) {
                    qDebug() << "尝试使用HTTP替代HTTPS:" << httpUrl;
                    url = QUrl(httpUrl);
                } else {
                    addTextMessage(":/res/img/home.png", senderName, "[图片]（系统不支持SSL）", isMine);
                    return;
                }
            }
        }
        
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest networkRequest;
        networkRequest.setUrl(url);
        
        // 如果是HTTPS，设置SSL配置以忽略证书错误
        if (url.scheme().toLower() == "https") {
            QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
            sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConfig.setProtocol(QSsl::TlsV1_2OrLater); // 使用TLS 1.2或更高版本
            networkRequest.setSslConfiguration(sslConfig);
        }
        
        QNetworkReply* reply = manager->get(networkRequest);
        
        // 忽略SSL错误（开发环境常用做法，如果图片URL是HTTPS）
        connect(reply, &QNetworkReply::sslErrors, this, [=](const QList<QSslError>& errors) {
            qDebug() << "SSL错误，忽略证书验证，URL:" << imageUrl << "，错误数:" << errors.size();
            for (const QSslError& error : errors) {
                qDebug() << "SSL错误详情:" << error.errorString();
            }
            reply->ignoreSslErrors();
        });
        
        connect(reply, &QNetworkReply::finished, this, [=, this]() {
            int error = reply->error();
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray imageData = reply->readAll();
                if (imageData.isEmpty()) {
                    qDebug() << "图片数据为空";
                    addTextMessage(":/res/img/home.png", senderName, "[图片]（数据为空）", isMine);
                } else {
                    QFile file(savePath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(imageData);
                        file.close();
                        
                        // 下载成功，显示图片
                        addImageMessage(":/res/img/home.png", senderName, savePath, isMine);
                        qDebug() << "图片下载成功:" << savePath << "，大小:" << imageData.size() << "字节";
                    } else {
                        qDebug() << "保存图片失败:" << savePath;
                        addTextMessage(":/res/img/home.png", senderName, "[图片]（保存失败）", isMine);
                    }
                }
            } else {
                QString qerror = reply->errorString();
                qDebug() << "图片下载失败，URL:" << imageUrl;
                qDebug() << "错误码:" << error << "，错误描述:" << qerror;
                addTextMessage(":/res/img/home.png", senderName, QString("[图片]（下载失败: %1）").arg(qerror), isMine);
            }
            
            reply->deleteLater();
            manager->deleteLater();
        });
    }
    
    // 使用SDK接口下载图片（优先使用，避免TLS问题）
    void downloadImageFromSDK(const QJsonObject& imageElem, const QString& savePath, const QString& senderName, bool isMine)
    {
        // 获取图片ID和URL
        QString imageId = imageElem[kTIMImageElemLargeId].toString();
        QString imageUrl = imageElem[kTIMImageElemLargeUrl].toString();
        
        if (imageId.isEmpty()) {
            imageId = imageElem[kTIMImageElemThumbId].toString();
            imageUrl = imageElem[kTIMImageElemThumbUrl].toString();
        }
        if (imageId.isEmpty()) {
            imageId = imageElem[kTIMImageElemOrigId].toString();
            imageUrl = imageElem[kTIMImageElemOrigUrl].toString();
        }
        
        // 优先尝试使用SDK的下载接口（推荐方式，避免TLS问题）
        // 如果URL和ID都不为空，使用SDK下载
        if (!imageUrl.isEmpty() && !imageId.isEmpty()) {
            // 构造SDK下载参数
            QJsonObject downloadParam;
            downloadParam[kTIMMsgDownloadElemParamUrl] = imageUrl;
            downloadParam[kTIMMsgDownloadElemParamId] = imageId;
            downloadParam[kTIMMsgDownloadElemParamType] = (int)kTIMDownload_File; // 使用File类型下载图片
            downloadParam[kTIMMsgDownloadElemParamFlag] = 0; // 默认值
            downloadParam[kTIMMsgDownloadElemParamBusinessId] = 0; // 默认值
            
            QJsonDocument doc(downloadParam);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            
            // 输出调试信息
            qDebug() << "=== SDK图片下载参数 ===";
            qDebug() << "图片URL:" << imageUrl;
            qDebug() << "图片ID:" << imageId;
            qDebug() << "下载参数JSON:" << QString::fromUtf8(jsonData);
            
            // 将保存路径转换为本地路径格式
            QString normalizedPath = QDir::toNativeSeparators(savePath);
            qDebug() << "保存路径:" << normalizedPath;
            
            // 创建超时定时器（在回调数据结构中保存，以便在回调中停止）
            QTimer* timeoutTimer = new QTimer(this);
            timeoutTimer->setSingleShot(true);
            
            // 创建回调数据结构
            struct DownloadImageCallbackData {
                ChatDialog* dlg;
                QString savePath;
                QString senderName;
                bool isMine;
                QString imageUrl; // 备用：如果SDK下载失败，使用HTTP下载
                QTimer* timeoutTimer; // 超时定时器，回调被调用时停止
            };
            DownloadImageCallbackData* callbackData = new DownloadImageCallbackData;
            callbackData->dlg = this;
            callbackData->savePath = normalizedPath;
            callbackData->senderName = senderName;
            callbackData->isMine = isMine;
            callbackData->imageUrl = imageUrl;
            callbackData->timeoutTimer = timeoutTimer;
            
            // 将路径转换为UTF-8编码的字节数组（Windows路径可能需要）
            QByteArray pathBytes = normalizedPath.toUtf8();
            
            qDebug() << "准备调用TIMMsgDownloadElemToPath...";
            
            // 调用SDK下载接口
            int ret = TIMMsgDownloadElemToPath(jsonData.constData(), pathBytes.constData(),
                [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                    qDebug() << "=== SDK下载回调函数被调用 ===";
                    qDebug() << "回调错误码:" << code;
                    qDebug() << "回调错误描述:" << (desc ? QString::fromUtf8(desc) : "无");
                    qDebug() << "回调JSON参数:" << (json_params ? QString::fromUtf8(json_params) : "无");
                    
                    DownloadImageCallbackData* data = (DownloadImageCallbackData*)user_data;
                    if (!data) {
                        qDebug() << "回调数据为空，退出";
                        return;
                    }
                    ChatDialog* dlg = data->dlg;
                    
                    // 停止超时定时器（如果回调被调用）
                    if (data->timeoutTimer) {
                        data->timeoutTimer->stop();
                        data->timeoutTimer->deleteLater();
                        data->timeoutTimer = nullptr;
                    }
                    
                    if (code == TIM_SUCC) {
                        // 下载成功，检查文件是否存在
                        QFile file(data->savePath);
                        if (file.exists()) {
                            // 显示图片
                            dlg->addImageMessage(":/res/img/home.png", data->senderName, data->savePath, data->isMine);
                            qDebug() << "SDK图片下载成功:" << data->savePath;
                        } else {
                            qDebug() << "SDK下载成功但文件不存在:" << data->savePath;
                            // 如果SDK下载失败，尝试HTTP下载
                            if (!data->imageUrl.isEmpty()) {
                                qDebug() << "尝试使用HTTP下载作为备用方案";
                                dlg->downloadImageFromUrl(data->imageUrl, data->savePath, data->senderName, data->isMine);
                            } else {
                                dlg->addTextMessage(":/res/img/home.png", data->senderName, "[图片]（下载失败）", data->isMine);
                            }
                        }
                    } else {
                        QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                        qDebug() << "SDK图片下载失败，错误码:" << code << "，描述:" << errorDesc;
                        // SDK下载失败，尝试HTTP下载作为备用方案
                        if (!data->imageUrl.isEmpty()) {
                            qDebug() << "SDK下载失败，尝试使用HTTP下载作为备用方案";
                            dlg->downloadImageFromUrl(data->imageUrl, data->savePath, data->senderName, data->isMine);
                        } else {
                            dlg->addTextMessage(":/res/img/home.png", data->senderName, QString("[图片]（下载失败: %1）").arg(errorDesc), data->isMine);
                        }
                    }
                    
                    delete data;
                }, callbackData);
            
            qDebug() << "TIMMsgDownloadElemToPath调用返回，错误码:" << ret << "（TIM_SUCC=" << TIM_SUCC << "）";
            
            if (ret != TIM_SUCC) {
                QString errorDesc = QString("错误码: %1").arg(ret);
                if (ret == -3) {
                    errorDesc = "JSON格式错误或字段错误";
                } else if (ret == -1) {
                    errorDesc = "参数错误";
                }
                qDebug() << "调用TIMMsgDownloadElemToPath失败，错误码:" << ret << "，错误描述:" << errorDesc;
                qDebug() << "注意：只有当返回TIM_SUCC(0)时，回调函数才会被调用";
                qDebug() << "JSON参数:" << QString::fromUtf8(jsonData);
                qDebug() << "路径参数:" << normalizedPath;
                // SDK接口调用失败，尝试HTTP下载
                if (!imageUrl.isEmpty()) {
                    qDebug() << "SDK接口调用失败，尝试使用HTTP下载作为备用方案";
                    downloadImageFromUrl(imageUrl, savePath, senderName, isMine);
                } else {
                    addTextMessage(":/res/img/home.png", senderName, QString("[图片]（无法下载: %1）").arg(errorDesc), isMine);
                }
                delete callbackData;
            } else {
                qDebug() << "TIMMsgDownloadElemToPath调用成功（返回TIM_SUCC），但回调可能不可靠，添加超时检查...";
                qDebug() << "回调函数将在下载完成或失败时被调用";
                
                // 添加超时检查：如果3秒后回调没有被调用，使用HTTP下载作为备用
                connect(timeoutTimer, &QTimer::timeout, this, [=]() {
                    // 检查文件是否已经下载成功
                    QFile file(normalizedPath);
                    if (!file.exists()) {
                        qDebug() << "SDK下载超时（3秒），文件不存在，使用HTTP下载作为备用方案";
                        if (!imageUrl.isEmpty()) {
                            downloadImageFromUrl(imageUrl, savePath, senderName, isMine);
                        } else {
                            addTextMessage(":/res/img/home.png", senderName, "[图片]（下载超时）", isMine);
                        }
                    } else {
                        qDebug() << "SDK下载超时，但文件已存在，显示图片";
                        addImageMessage(":/res/img/home.png", senderName, normalizedPath, isMine);
                    }
                    timeoutTimer->deleteLater();
                });
                timeoutTimer->start(3000); // 3秒超时
                
                // 注意：回调函数会在下载完成或失败时被调用，如果回调被调用，应该停止定时器
                // 但由于回调可能不会被调用，我们依赖超时机制
            }
        } else if (!imageUrl.isEmpty()) {
            // 只有URL，没有ID，直接使用HTTP下载
            qDebug() << "图片只有URL，使用HTTP下载";
            downloadImageFromUrl(imageUrl, savePath, senderName, isMine);
        } else {
            qDebug() << "图片URL和ID都为空，无法下载";
            addTextMessage(":/res/img/home.png", senderName, "[图片]（无法获取）", isMine);
        }
    }

    // 更新文件上传进度
    void updateFileUploadProgress(uint32_t cur_size, uint32_t total_size)
    {
        if (total_size == 0) return;
        
        // 更新所有正在上传的文件进度
        for (auto it = m_fileUploadProgressMap.begin(); it != m_fileUploadProgressMap.end(); ++it) {
            QProgressBar* progressBar = it.value().first;
            QLabel* statusLabel = it.value().second;
            
            if (progressBar) {
                int progress = (int)((double)cur_size / total_size * 100);
                progressBar->setValue(progress);
                
                // 更新状态标签显示进度
                if (statusLabel) {
                    QString sizeStr = cur_size < 1024 ? QString("%1 字节").arg(cur_size) :
                                     cur_size < 1024 * 1024 ? QString("%1 KB").arg(cur_size / 1024.0, 0, 'f', 1) :
                                     QString("%1 MB").arg(cur_size / (1024.0 * 1024.0), 0, 'f', 1);
                    QString totalStr = total_size < 1024 ? QString("%1 字节").arg(total_size) :
                                      total_size < 1024 * 1024 ? QString("%1 KB").arg(total_size / 1024.0, 0, 'f', 1) :
                                      QString("%1 MB").arg(total_size / (1024.0 * 1024.0), 0, 'f', 1);
                    statusLabel->setText(QString("上传中: %1 / %2 (%3%)").arg(sizeStr).arg(totalStr).arg(progress));
                    statusLabel->setStyleSheet("color: blue; font-size: 10px;");
                }
            }
        }
    }

    TaQTWebSocket* m_pWs = NULL;
    QString m_unique_group_id;
    bool m_iGroupOwner = false;
    // 文件上传进度映射：文件路径 -> (进度条, 状态标签)
    QMap<QString, QPair<QProgressBar*, QLabel*>> m_fileUploadProgressMap;
    // 文件下载进度映射：文件路径 -> (进度条, 状态标签)
    QMap<QString, QPair<QProgressBar*, QLabel*>> m_fileDownloadProgressMap;
    
    // 文件下载信息结构（用于重试）
    struct FileDownloadInfo {
        QString fileUrl;           // 文件URL
        QJsonObject fileElem;      // 文件元素JSON（用于SDK下载）
        QString savePath;          // 保存路径
        QString senderName;        // 发送者名称
        QString fileName;          // 文件名
        int fileSize;              // 文件大小
        bool isMine;               // 是否是自己发送的
        QProgressBar* progressBar; // 进度条指针
        QLabel* statusLabel;       // 状态标签指针
    };
    // 文件下载信息映射：文件路径 -> 下载信息（用于重试）
    QMap<QString, FileDownloadInfo> m_fileDownloadInfoMap;
};
