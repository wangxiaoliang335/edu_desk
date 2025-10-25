#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
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
#include "TaQTWebSocket.h"
#include "CommonInfo.h"

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
                addTextMessage(":/res/img/home.png", "我", iter.content, false);
            }
        }
    }

private slots:
    void sendMyTextMessage()
    {
        QString text = m_lineEdit->text().trimmed();
        if (text.isEmpty()) return;

        UserInfo userinfo = CommonInfo::GetData();
        addTextMessage(":/res/img/home.png", userinfo.strName, text, true);
        m_lineEdit->clear();


        QJsonObject createGroupMsg;
        createGroupMsg["type"] = "5";
        createGroupMsg["content"] = text;
        createGroupMsg["unique_group_id"] = m_unique_group_id;
        createGroupMsg["sender_id"] = userinfo.teacher_unique_id;
        createGroupMsg["sender_name"] = userinfo.strName;
        createGroupMsg["groupowner"] = m_iGroupOwner;

        // 用 QJsonDocument 序列化
        QJsonDocument doc(createGroupMsg);
        // 输出美化格式（有缩进）
        QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
        qDebug() << "美化格式:" << prettyString;

        if (!prettyString.isEmpty()) {
            //socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
            TaQTWebSocket::sendPrivateMessage(QString("to:%1:%2").arg(m_unique_group_id, prettyString));
        }

        // 模拟回复
        //QTimer::singleShot(1200, this, [=]() {
        //    addVoiceMessage(":/avatar_teacher2.png", "语文老师", 5, false);
        //    });
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
        QString imgPath = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
        if (!imgPath.isEmpty())
            addImageMessage(":/res/img/home.png", "我", imgPath, true);
    }

    void sendMyFileMessage()
    {
        QString filePath = QFileDialog::getOpenFileName(this, "选择文件");
        if (!filePath.isEmpty())
            addFileMessage(":/res/img/home.png", "我", filePath, true);
    }

    void sendMyVoiceMessage()
    {
        // 模拟 8秒语音
        addVoiceMessage(":/res/img/home.png", "我", 8, true);
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

    QWidget* buildMessageWidget(const QString& avatarPath, QWidget* contentWidget, bool isMine, bool hideAvatar)
    {
        QWidget* msgWidget = new QWidget();
        QVBoxLayout* vLayout = new QVBoxLayout(msgWidget);
        vLayout->setContentsMargins(5, 5, 5, 5);

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

        QWidget* msgWidget = buildMessageWidget(avatarPath, lblMessage, isMine, hideAvatar);

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

        QWidget* msgWidget = buildMessageWidget(avatarPath, lblImage, isMine, hideAvatar);

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

        QWidget* msgWidget = buildMessageWidget(avatarPath, lblFile, isMine, hideAvatar);

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

        QWidget* msgWidget = buildMessageWidget(avatarPath, lblVoice, isMine, hideAvatar);

        QListWidgetItem* item = new QListWidgetItem(m_listWidget);
        item->setSizeHint(msgWidget->sizeHint());
        m_listWidget->addItem(item);
        m_listWidget->setItemWidget(item, msgWidget);

        m_lastMessage = { avatarPath, senderName, QString("[语音] %1秒").arg(seconds), isMine, now };
        m_hasLastMessage = true;
        m_listWidget->scrollToBottom();
    }

    TaQTWebSocket* m_pWs = NULL;
    QString m_unique_group_id;
    bool m_iGroupOwner = false;
};
