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
#include <QTimer>
#include <QDateTime>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonParseError>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <QMessageBox>
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
protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit clicked();
        QLabel::mousePressEvent(event);
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
            // 先显示在界面上
            addFileMessage(":/res/img/home.png", userinfo.strName, filePath, true);
            // 使用腾讯SDK发送文件
            sendFileMessageViaTIMSDK(filePath, userinfo);
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
        connect(lblImage, &ClickableLabelEx::clicked, this, [=]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(imgPath));
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
        // TODO: 实现图片上传和发送
        // 图片消息需要先上传图片，然后发送图片元素
        qDebug() << "发送图片消息（待实现）:" << imgPath;
    }
    
    // 使用腾讯SDK发送文件消息
    void sendFileMessageViaTIMSDK(const QString& filePath, const UserInfo& userinfo)
    {
        // TODO: 实现文件上传和发送
        qDebug() << "发送文件消息（待实现）:" << filePath;
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
                    // TODO: 处理图片消息
                    addTextMessage(":/res/img/home.png", senderName, "[图片]", false);
                } else if (elemType == kTIMElem_File) {
                    // TODO: 处理文件消息
                    addTextMessage(":/res/img/home.png", senderName, "[文件]", false);
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
                                    dlg->addTextMessage(":/res/img/home.png", senderName, "[图片]", isMine);
                                } else if (elemType == kTIMElem_File) {
                                    dlg->addTextMessage(":/res/img/home.png", senderName, "[文件]", isMine);
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

    TaQTWebSocket* m_pWs = NULL;
    QString m_unique_group_id;
    bool m_iGroupOwner = false;
};
