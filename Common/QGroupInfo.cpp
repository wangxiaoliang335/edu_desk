#include "QGroupInfo.h"
#include "ClassTeacherDialog.h"
#include "ClassTeacherDelDialog.h"

// 解散群聊回调数据结构
struct DismissGroupCallbackData {
    QGroupInfo* dlg;
    QString groupId;
    QString userId;
    QString userName;
};

QGroupInfo::QGroupInfo(QWidget* parent)
	: QDialog(parent)
{
    
}

void QGroupInfo::initData(QString groupName, QString groupNumberId)
{
    setWindowTitle("班级管理");
    resize(300, 600);
    setStyleSheet("QPushButton { font-size:14px; } QLabel { font-size:14px; }");

    m_groupName = groupName;
    m_groupNumberId = groupNumberId;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    m_classTeacherDlg = new ClassTeacherDialog(this);
    m_classTeacherDelDlg = new ClassTeacherDelDialog(this);
    m_courseDlg = new CourseDialog();
    m_courseDlg->setWindowTitle("课程表");
    m_courseDlg->resize(800, 600);

    // 设置一些课程
    //m_courseDlg->setCourse(1, 0, "数学");
    //m_courseDlg->setCourse(1, 1, "音乐");
    //m_courseDlg->setCourse(2, 1, "语文", true); // 高亮
    //m_courseDlg->setCourse(3, 4, "体育", true);

    m_classTeacherDlg->setWindowTitle(" ");

    // 顶部用户信息
    QHBoxLayout* topLayout = new QHBoxLayout;
    //QLabel* lblNumber = new QLabel("①");
    //lblNumber->setAlignment(Qt::AlignCenter);
    //lblNumber->setFixedSize(30, 30);
    //lblNumber->setStyleSheet("background-color: yellow; border-radius: 15px; font-weight: bold;");
    QLabel* lblAvatar = new QLabel;
    lblAvatar->setFixedSize(50, 50);
    lblAvatar->setStyleSheet("background-color: lightgray;");
    QLabel* lblInfo = new QLabel(groupName + "\n" + groupNumberId);
    QPushButton* btnMore = new QPushButton("...");
    btnMore->setFixedSize(30, 30);
    //topLayout->addWidget(lblNumber);
    topLayout->addWidget(lblAvatar);
    topLayout->addWidget(lblInfo, 1);
    topLayout->addStretch();
    topLayout->addWidget(btnMore);
    mainLayout->addLayout(topLayout);

    // 班级编号
    QLineEdit* editClassNum = new QLineEdit("2349235");
    editClassNum->setAlignment(Qt::AlignCenter);
    editClassNum->setStyleSheet("color:red; font-size:18px; font-weight:bold;");
    mainLayout->addWidget(editClassNum);

    QHBoxLayout* pHBoxLayut = new QHBoxLayout(this);
    // 班级课程表按钮
    QPushButton* btnSchedule = new QPushButton("班级课程表");
    btnSchedule->setStyleSheet("background-color:green; color:white; font-weight:bold;");

    pHBoxLayut->addWidget(btnSchedule);
    pHBoxLayut->addStretch(2);
    mainLayout->addLayout(pHBoxLayut);

    connect(btnSchedule, &QPushButton::clicked, this, [=]() {
        qDebug() << "红框区域被点击！";
        if (m_courseDlg)
        {
            m_courseDlg->show();
        }

        // 这里可以弹出输入框、打开聊天功能等
        /*if (m_chatDlg)
        {
            m_chatDlg->show();
        }*/
    });
    

    // 好友列表
    QGroupBox* groupFriends = new QGroupBox("好友列表");
    QVBoxLayout* friendsLayout = new QVBoxLayout(groupFriends);
    circlesLayout = new QHBoxLayout;

    //QString blueStyle = "background-color:blue; border-radius:15px; color:white; font-weight:bold;";
    //QString redStyle = "background-color:red; border-radius:15px; color:white; font-weight:bold;";

    //FriendButton* circleRed = new FriendButton("", this);
    //circleRed->setStyleSheet(redStyle);
    //FriendButton* circleBlue1 = new FriendButton("", this);
    //FriendButton* circleBlue2 = new FriendButton("", this);
    //FriendButton* circlePlus = new FriendButton("+", this);
    //FriendButton* circleMinus = new FriendButton("-", this);

    //// 接收右键菜单信号
    //connect(circleRed, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleRed, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue1, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue1, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue2, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue2, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });

    //circlesLayout->addWidget(circleRed);
    //circlesLayout->addWidget(circleBlue1);
    //circlesLayout->addWidget(circleBlue2);
    //circlesLayout->addWidget(circlePlus);
    //circlesLayout->addWidget(circleMinus);

    friendsLayout->addLayout(circlesLayout);
    mainLayout->addWidget(groupFriends);

    // 科目输入
    QGroupBox* groupSubject = new QGroupBox("科目");
    QVBoxLayout* subjectLayout = new QVBoxLayout(groupSubject);
    QLineEdit* editSubject = new QLineEdit("语文");
    editSubject->setAlignment(Qt::AlignCenter);
    editSubject->setStyleSheet("color:red; font-size:18px; font-weight:bold;");
    subjectLayout->addWidget(editSubject);
    mainLayout->addWidget(groupSubject);

    // 开启对讲(开关)
    QHBoxLayout* talkLayout = new QHBoxLayout;
    QLabel* lblTalk = new QLabel("开启对讲");
    QCheckBox* chkTalk = new QCheckBox;
    talkLayout->addWidget(lblTalk);
    talkLayout->addStretch();
    talkLayout->addWidget(chkTalk);
    mainLayout->addLayout(talkLayout);

    // 解散群聊 / 退出群聊
    QHBoxLayout* bottomBtns = new QHBoxLayout;
    m_btnDismiss = new QPushButton("解散群聊");
    m_btnExit = new QPushButton("退出群聊");
    bottomBtns->addWidget(m_btnDismiss);
    bottomBtns->addWidget(m_btnExit);
    mainLayout->addLayout(bottomBtns);
    
    // 初始状态：默认都禁用，等InitGroupMember调用后再更新
    m_btnDismiss->setEnabled(false);
    m_btnExit->setEnabled(false);
    
    // 连接退出群聊按钮的点击事件
    connect(m_btnExit, &QPushButton::clicked, this, &QGroupInfo::onExitGroupClicked);
    // 连接解散群聊按钮的点击事件
    connect(m_btnDismiss, &QPushButton::clicked, this, &QGroupInfo::onDismissGroupClicked);
}

void QGroupInfo::InitGroupMember(QVector<GroupMemberInfo> groupMemberInfo)
{
    m_groupMemberInfo = groupMemberInfo;

    // 清空之前的成员圆圈（保留 + 和 - 按钮）
    if (circlesLayout) {
        // 先找到 + 和 - 按钮，保存它们
        FriendButton* circlePlus = nullptr;
        FriendButton* circleMinus = nullptr;
        
        // 遍历布局，找到 + 和 - 按钮，删除其他成员圆圈
        QList<QLayoutItem*> itemsToRemove;
        for (int i = 0; i < circlesLayout->count(); i++) {
            QLayoutItem* item = circlesLayout->itemAt(i);
            if (item && item->widget()) {
                FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
                if (btn) {
                    if (btn->text() == "+") {
                        circlePlus = btn;
                    } else if (btn->text() == "-") {
                        circleMinus = btn;
                    } else {
                        // 成员圆圈，标记为删除
                        itemsToRemove.append(item);
                    }
                }
            }
        }
        
        // 删除成员圆圈
        for (auto item : itemsToRemove) {
            circlesLayout->removeItem(item);
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        
        // 如果 + 和 - 按钮不存在，创建它们
        if (!circlePlus) {
            circlePlus = new FriendButton("+", this);
            connect(circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_classTeacherDlg && m_classTeacherDlg->isHidden())
                {
                    m_classTeacherDlg->show();
                }
                else
                {
                    m_classTeacherDlg->hide();
                }
            });
            circlesLayout->addWidget(circlePlus);
        }
        if (!circleMinus) {
            circleMinus = new FriendButton("-", this);
            connect(circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_classTeacherDelDlg && m_classTeacherDelDlg->isHidden())
                {
                    m_classTeacherDelDlg->show();
                }
                else
                {
                    m_classTeacherDelDlg->hide();
                }
            });
            circlesLayout->addWidget(circleMinus);
        }
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 添加成员圆圈（在 + 和 - 按钮之前插入）
    // 找到 + 按钮的位置，在它之前插入成员
    int insertIndex = circlesLayout->count() - 2; // 在倒数第二个位置（+ 按钮之前）插入
    if (insertIndex < 0) insertIndex = 0;
    
    for (auto iter : m_groupMemberInfo)
    {
        FriendButton* circleBtn = nullptr;
        if (iter.member_role == "群主")
        {
            circleBtn = new FriendButton("", this);
            circleBtn->setStyleSheet(redStyle); // 群主用红色圆圈
        }
        else
        {
            circleBtn = new FriendButton("", this);
            circleBtn->setStyleSheet(blueStyle); // 其他成员用蓝色圆圈
        }
        
        // 接收右键菜单信号
        connect(circleBtn, &FriendButton::setLeaderRequested, this, []() {
            qDebug() << "设为班主任";
        });
        connect(circleBtn, &FriendButton::cancelLeaderRequested, this, []() {
            qDebug() << "取消班主任";
        });
        
        circleBtn->setText(iter.member_name);
        circleBtn->setProperty("member_id", iter.member_id);
        
        // 在 + 按钮之前插入成员圆圈
        circlesLayout->insertWidget(insertIndex, circleBtn);
        insertIndex++; // 更新插入位置
    }
    
    // 更新按钮状态（根据当前用户是否是群主）
    updateButtonStates();
}

void QGroupInfo::updateButtonStates()
{
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 查找当前用户在成员列表中的角色
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId) {
            if (member.member_role == "群主") {
                isOwner = true;
            }
            break;
        }
    }
    
    // 根据当前用户是否是群主来设置按钮状态
    if (m_btnDismiss && m_btnExit) {
        if (isOwner) {
            // 当前用户是群主：解散群聊按钮可用，退出群聊按钮禁用
            m_btnDismiss->setEnabled(true);
            m_btnExit->setEnabled(false);
            qDebug() << "当前用户是群主，解散群聊按钮可用，退出群聊按钮禁用";
        } else {
            // 当前用户不是群主：解散群聊按钮禁用，退出群聊按钮可用
            m_btnDismiss->setEnabled(false);
            m_btnExit->setEnabled(true);
            qDebug() << "当前用户不是群主，解散群聊按钮禁用，退出群聊按钮可用";
        }
    }
}

void QGroupInfo::onExitGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        QMessageBox::warning(this, "错误", "群组ID为空，无法退出群聊");
        return;
    }
    
    // 确认对话框
    int ret = QMessageBox::question(this, "确认退出", 
        QString("确定要退出群聊 \"%1\" 吗？").arg(m_groupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    qDebug() << "开始退出群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 构造回调数据结构
    struct ExitGroupCallbackData {
        QGroupInfo* dlg;
        QString groupId;
        QString userId;
        QString userName;
    };
    
    ExitGroupCallbackData* callbackData = new ExitGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupId = m_groupNumberId;
    callbackData->userId = userId;
    callbackData->userName = userName;
    
    // 调用腾讯SDK的退出群聊接口
    QByteArray groupIdBytes = m_groupNumberId.toUtf8();
    int retCode = TIMGroupQuit(groupIdBytes.constData(),
        [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
            ExitGroupCallbackData* data = (ExitGroupCallbackData*)user_data;
            QGroupInfo* dlg = data->dlg;
            
            if (code == TIM_SUCC) {
                qDebug() << "腾讯SDK退出群聊成功:" << data->groupId;
                
                // 立即从本地成员列表中移除当前用户
                QVector<GroupMemberInfo> updatedMemberList;
                QString leftUserId = data->userId; // 记录退出的用户ID
                for (const auto& member : dlg->m_groupMemberInfo) {
                    if (member.member_id != data->userId) {
                        updatedMemberList.append(member);
                    }
                }
                dlg->m_groupMemberInfo = updatedMemberList;
                
                // 立即更新UI（移除退出的成员）
                dlg->InitGroupMember(dlg->m_groupMemberInfo);
                
                // 腾讯SDK成功，现在调用自己的服务器接口（传递退出的用户ID，用于后续处理）
                dlg->sendExitGroupRequestToServer(data->groupId, data->userId, leftUserId);
            } else {
                QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                QString errorMsg = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc);
                qDebug() << errorMsg;
                QMessageBox::critical(dlg, "退出失败", errorMsg);
            }
            
            // 释放回调数据
            delete data;
        }, callbackData);
    
    if (retCode != TIM_SUCC) {
        QString errorMsg = QString("调用TIMGroupQuit接口失败，错误码: %1").arg(retCode);
        qDebug() << errorMsg;
        QMessageBox::critical(this, "错误", errorMsg);
        delete callbackData;
    }
}

void QGroupInfo::sendExitGroupRequestToServer(const QString& groupId, const QString& userId, const QString& leftUserId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/leave";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "退出群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出成员退出群聊信号，通知父窗口刷新成员列表（传递退出的用户ID）
                    emit this->memberLeftGroup(groupId, leftUserId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "退出成功", 
                        QString("已成功退出群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    QMessageBox::warning(this, "退出失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                QMessageBox::warning(this, "退出失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送退出群聊请求到服务器失败:" << reply->errorString();
            QMessageBox::warning(this, "退出失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::onDismissGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        QMessageBox::warning(this, "错误", "群组ID为空，无法解散群聊");
        return;
    }
    
    // 确认对话框
    int ret = QMessageBox::question(this, "确认解散", 
        QString("确定要解散群聊 \"%1\" 吗？\n解散后所有成员将被移除，此操作不可恢复！").arg(m_groupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    qDebug() << "开始解散群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 构造回调数据结构
    DismissGroupCallbackData* callbackData = new DismissGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupId = m_groupNumberId;
    callbackData->userId = userId;
    callbackData->userName = userName;
    
    // 调用腾讯SDK的解散群聊接口
    QByteArray groupIdBytes = m_groupNumberId.toUtf8();
    int retCode = TIMGroupDelete(groupIdBytes.constData(),
        [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
            DismissGroupCallbackData* data = (DismissGroupCallbackData*)user_data;
            QGroupInfo* dlg = data->dlg;
            
            if (code == TIM_SUCC) {
                qDebug() << "腾讯SDK解散群聊成功:" << data->groupId;
                
                // 腾讯SDK成功，现在调用自己的服务器接口（传递回调数据以便后续释放）
                dlg->sendDismissGroupRequestToServer(data->groupId, data->userId, data);
            } else {
                QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                QString errorMsg = QString("解散群聊失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc);
                qDebug() << errorMsg;
                QMessageBox::critical(dlg, "解散失败", errorMsg);
                
                // 释放回调数据
                delete data;
            }
        }, callbackData);
    
    if (retCode != TIM_SUCC) {
        QString errorMsg = QString("调用TIMGroupDelete接口失败，错误码: %1").arg(retCode);
        qDebug() << errorMsg;
        QMessageBox::critical(this, "错误", errorMsg);
        delete callbackData;
    }
}

void QGroupInfo::sendDismissGroupRequestToServer(const QString& groupId, const QString& userId, void* callbackData)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器（假设解散接口为 /groups/dismiss，如果不同请修改）
    QString url = "http://47.100.126.194:5000/groups/dismiss";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "解散群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出群聊解散信号，通知父窗口刷新群列表
                    emit this->groupDismissed(groupId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "解散成功", 
                        QString("已成功解散群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    QMessageBox::warning(this, "解散失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                QMessageBox::warning(this, "解散失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送解散群聊请求到服务器失败:" << reply->errorString();
            QMessageBox::warning(this, "解散失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        // 释放回调数据（如果提供了）
        if (callbackData) {
            delete (DismissGroupCallbackData*)callbackData;
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

QGroupInfo::~QGroupInfo()
{}

