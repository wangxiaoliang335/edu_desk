#pragma execution_character_set("utf-8")
#include "FriendSelectDialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QScrollArea>
#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

FriendSelectDialog::FriendSelectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("选择好友");
    resize(400, 500);
    setStyleSheet("background-color:#555555; font-size:14px;");

    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, &FriendSelectDialog::onHttpSuccess);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &FriendSelectDialog::onHttpFailed);
    }
    
    // 初始化REST API
    m_restAPI = new TIMRestAPI(this);
    // 注意：管理员账号信息在使用REST API时再设置，因为此时用户可能还未登录

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 标题
    QLabel* lblTitle = new QLabel("好友列表");
    lblTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
    mainLayout->addWidget(lblTitle);

    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("background-color:transparent; border:none;");

    m_scrollWidget = new QWidget;
    m_friendsLayout = new QVBoxLayout(m_scrollWidget);
    m_friendsLayout->setSpacing(8);
    m_friendsLayout->setContentsMargins(5, 5, 5, 5);
    m_friendsLayout->addStretch();

    m_scrollArea->setWidget(m_scrollWidget);
    mainLayout->addWidget(m_scrollArea);

    // 底部按钮
    QHBoxLayout* bottomLayout = new QHBoxLayout;
    QPushButton* btnCancel = new QPushButton("取消");
    QPushButton* btnOk = new QPushButton("确定");
    btnCancel->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
    btnOk->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnCancel);
    bottomLayout->addWidget(btnOk);
    mainLayout->addLayout(bottomLayout);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnOk, &QPushButton::clicked, this, &FriendSelectDialog::onOkClicked);
}

FriendSelectDialog::~FriendSelectDialog()
{}

void FriendSelectDialog::InitData()
{
    UserInfo userInfo = CommonInfo::GetData();
    if (m_httpHandler)
    {
        QString url = "http://47.100.126.194:5000/friends?";
        url += "id_card=";
        url += userInfo.strIdNumber;
        m_httpHandler->get(url);
    }
}

void FriendSelectDialog::setExcludedMemberIds(const QVector<QString>& memberIds)
{
    m_excludedMemberIds = memberIds;
}

void FriendSelectDialog::setGroupId(const QString& groupId)
{
    m_groupId = groupId;
}

void FriendSelectDialog::setGroupName(const QString& groupName)
{
    m_groupName = groupName;
}

QVector<QString> FriendSelectDialog::getSelectedFriendIds()
{
    QVector<QString> selectedIds;
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString teacherUniqueId = checkbox->property("teacher_unique_id").toString();
            if (!teacherUniqueId.isEmpty())
            {
                selectedIds.append(teacherUniqueId);
            }
        }
    }
    return selectedIds;
}

void FriendSelectDialog::onHttpSuccess(const QString& responseString)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
    if (jsonDoc.isObject())
    {
        QJsonObject obj = jsonDoc.object();

        clearLayout(m_friendsLayout);

        if (obj["friends"].isArray())
        {
            QJsonArray friendsArray = obj.value("friends").toArray();
            
            for (int i = 0; i < friendsArray.size(); i++)
            {
                QJsonObject friendObj = friendsArray.at(i).toObject();

                // teacher_info 对象
                QJsonObject teacherInfo = friendObj.value("teacher_info").toObject();
                QString name = teacherInfo.value("name").toString();
                QString teacher_unique_id = teacherInfo.value("teacher_unique_id").toString();

                // 检查是否在排除列表中（通过 teacher_unique_id 匹配）
                bool isExcluded = false;
                for (const QString& excludedId : m_excludedMemberIds)
                {
                    if (excludedId == teacher_unique_id)
                    {
                        isExcluded = true;
                        break;
                    }
                }

                // 如果不在排除列表中，才显示这个好友
                if (!isExcluded)
                {
                    // user_details 对象
                    QJsonObject userDetails = friendObj.value("user_details").toObject();
                    QString phone = userDetails.value("phone").toString();
                    QString uname = userDetails.value("name").toString();

                    // 创建复选框
                    QCheckBox* checkbox = new QCheckBox(name.isEmpty() ? uname : name, m_scrollWidget);
                    checkbox->setProperty("teacher_unique_id", teacher_unique_id); // 使用teacher_unique_id作为唯一标识
                    checkbox->setStyleSheet("color:white; padding:4px;");
                    
                    // 存储好友信息：teacher_unique_id -> name
                    m_friendInfoMap[teacher_unique_id] = name.isEmpty() ? uname : name;
                    
                    m_checkBoxes.append(checkbox);
                    m_friendsLayout->insertWidget(m_friendsLayout->count() - 1, checkbox); // 在stretch之前插入
                }
            }
        }

        QJsonObject oTmp = obj["data"].toObject();
        QString strTmp = oTmp["message"].toString();
        qDebug() << "status:" << oTmp["code"].toString();
        qDebug() << "msg:" << oTmp["message"].toString();
    }
    else
    {
        qDebug() << "网络错误：JSON解析失败";
    }
}

void FriendSelectDialog::onHttpFailed(const QString& errResponseString)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
    if (jsonDoc.isObject())
    {
        QJsonObject obj = jsonDoc.object();
        if (obj["data"].isObject())
        {
            QJsonObject oTmp = obj["data"].toObject();
            QString strTmp = oTmp["message"].toString();
            qDebug() << "status:" << oTmp["code"].toString();
            qDebug() << "msg:" << oTmp["message"].toString();
        }
    }
}

// 将TIMResult错误码转换为错误描述字符串
static QString getTIMResultErrorString(int errorCode) {
    switch (errorCode) {
        case TIM_SUCC:
            return "成功";
        case TIM_ERR_SDKUNINIT:
            return "ImSDK未初始化";
        case TIM_ERR_NOTLOGIN:
            return "用户未登录";
        case TIM_ERR_JSON:
            return "错误的Json格式或Json Key - 请检查JSON参数格式是否正确，特别是字段名称和数据类型";
        case TIM_ERR_PARAM:
            return "参数错误 - 请检查传入的参数是否有效";
        case TIM_ERR_CONV:
            return "无效的会话";
        case TIM_ERR_GROUP:
            return "无效的群组";
        default:
            return QString("未知错误码: %1").arg(errorCode);
    }
}

void FriendSelectDialog::onOkClicked()
{
    // 获取选中的好友
    QVector<QString> selectedTeacherIds; // teacher_unique_id列表
    QVector<QString> selectedNames; // 好友名称列表
    
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox && checkbox->isChecked())
        {
            QString teacherUniqueId = checkbox->property("teacher_unique_id").toString();
            
            if (!teacherUniqueId.isEmpty())
            {
                selectedTeacherIds.append(teacherUniqueId);
                
                // 从map中获取名称
                if (m_friendInfoMap.contains(teacherUniqueId))
                {
                    selectedNames.append(m_friendInfoMap[teacherUniqueId]);
                }
                else
                {
                    selectedNames.append(teacherUniqueId);
                }
            }
        }
    }
    
    if (selectedTeacherIds.isEmpty())
    {
        QMessageBox::information(this, "提示", "请至少选择一个好友");
        return;
    }
    
    if (m_groupId.isEmpty())
    {
        QMessageBox::warning(this, "错误", "群组ID未设置");
        return;
    }
    
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 在使用REST API前设置管理员账号信息
    // 注意：REST API需要使用应用管理员账号，使用当前登录用户的teacher_unique_id
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    }
    
    // 构造成员ID数组（REST API格式）
    QJsonArray memberArray;
    for (const QString& teacherId : selectedTeacherIds)
    {
        memberArray.append(teacherId);
    }
    
    // 创建回调数据结构
    struct InviteCallbackData {
        FriendSelectDialog* dlg;
        QVector<QString> memberIds;
        QVector<QString> memberNames;
        QString groupId;
    };
    
    InviteCallbackData* callbackData = new InviteCallbackData;
    callbackData->dlg = this;
    callbackData->memberIds = selectedTeacherIds;
    callbackData->memberNames = selectedNames;
    callbackData->groupId = m_groupId;
    
    qDebug() << "========== 邀请成员 - REST API ==========";
    qDebug() << "群组ID:" << m_groupId;
    qDebug() << "成员数量:" << memberArray.size();
    
    // 调用REST API邀请成员
    m_restAPI->inviteGroupMember(m_groupId, memberArray,
        [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg;
                
                // 特殊处理常见的错误码
                if (errorCode == 10007) {
                    errorMsg = QString("邀请成员失败\n错误码: %1\n错误描述: %2\n\n该群组未启用邀请功能，请检查群组类型和设置。").arg(errorCode).arg(errorDesc);
                } else {
                    errorMsg = QString("邀请成员失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                }
                
                qDebug() << errorMsg;
                QMessageBox::critical(callbackData->dlg, "邀请失败", errorMsg);
                delete callbackData;
                return;
            }
            
            // 邀请成功
            qDebug() << "邀请成员成功";
            
            // 调用服务器接口邀请成员
            callbackData->dlg->inviteMembersToServer(callbackData->memberIds, callbackData->memberNames);
            
            QMessageBox::information(callbackData->dlg, "邀请成功", "好友邀请已发送");
            callbackData->dlg->accept(); // 关闭对话框
            
            delete callbackData;
        });
}

void FriendSelectDialog::inviteMembersToServer(const QVector<QString>& memberIds, const QVector<QString>& memberNames)
{
    if (memberIds.size() != memberNames.size()) {
        qWarning() << "成员ID和名称数量不匹配";
        return;
    }
    
    if (memberIds.isEmpty()) {
        // 如果没有成员需要上传，直接发出信号
        emit membersInvitedSuccess(m_groupId);
        return;
    }
    
    UserInfo userInfo = CommonInfo::GetData();
    QString url = "http://47.100.126.194:5000/groups/sync";
    
    // 使用共享指针来跟踪所有请求的状态
    struct UploadState {
        int totalCount;
        int successCount;
        int failCount;
        FriendSelectDialog* dlg;
        QString groupId;
    };
    
    UploadState* state = new UploadState;
    state->totalCount = memberIds.size();
    state->successCount = 0;
    state->failCount = 0;
    state->dlg = this;
    state->groupId = m_groupId;
    
    // 为每个被邀请的成员创建群组信息并上传
    for (int i = 0; i < memberIds.size(); i++) {
        QString memberId = memberIds[i];
        QString memberName = memberNames[i];
        
        // 构造群组信息对象
        QJsonObject groupObj;
        groupObj["group_id"] = m_groupId;
        groupObj["group_name"] = m_groupName; // 使用设置的群组名称
        groupObj["group_type"] = kTIMGroup_Public; // 公开群（支持设置管理员）
        groupObj["face_url"] = "";
        groupObj["info_seq"] = 0;
        groupObj["latest_seq"] = 0;
        groupObj["is_shutup_all"] = false;
        
        // 群组详细信息
        groupObj["detail_group_id"] = m_groupId;
        groupObj["detail_group_name"] = m_groupName; // 使用设置的群组名称
        groupObj["detail_group_type"] = kTIMGroup_Private; // 私有群（只有私有群可以直接拉用户入群）
        groupObj["detail_face_url"] = "";
        groupObj["create_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        groupObj["detail_info_seq"] = 0;
        groupObj["introduction"] = "";
        groupObj["notification"] = "";
        groupObj["last_info_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        groupObj["last_msg_time"] = 0;
        groupObj["next_msg_seq"] = 0;
        groupObj["member_num"] = 0; // 服务器会更新
        groupObj["max_member_num"] = 2000;
        groupObj["online_member_num"] = 0;
        groupObj["owner_identifier"] = userInfo.teacher_unique_id;
        groupObj["add_option"] = kTIMGroupAddOpt_Any;
        groupObj["visible"] = 2;
        groupObj["searchable"] = 2;
        groupObj["detail_is_shutup_all"] = false;
        
        // 被邀请成员的member_info
        QJsonObject memberInfo;
        memberInfo["user_id"] = memberId;
        memberInfo["readed_seq"] = 0;
        memberInfo["msg_flag"] = 0;
        memberInfo["join_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        memberInfo["self_role"] = (int)kTIMMemberRole_Normal;
        memberInfo["self_msg_flag"] = 0;
        memberInfo["unread_num"] = 0;
        memberInfo["user_name"] = memberName;
        
        groupObj["member_info"] = memberInfo;
        
        // 构造groups数组
        QJsonArray groupsArray;
        groupsArray.append(groupObj);
        
        // 构造上传JSON
        QJsonObject uploadData;
        uploadData["user_id"] = memberId;
        uploadData["groups"] = groupsArray;
        
        // 转换为JSON字符串
        QJsonDocument uploadDoc(uploadData);
        QByteArray jsonData = uploadDoc.toJson(QJsonDocument::Compact);
        
        // 上传到服务器
        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest request;
        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply* reply = manager->post(request, jsonData);
        
        connect(reply, &QNetworkReply::finished, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "上传被邀请成员群组信息到服务器成功，成员ID:" << memberId;
                state->successCount++;
            } else {
                qWarning() << "上传被邀请成员群组信息失败，成员ID:" << memberId << "错误:" << reply->errorString();
                state->failCount++;
            }
            
            // 检查是否所有请求都已完成
            if (state->successCount + state->failCount >= state->totalCount) {
                // 所有请求都已完成，发出信号通知刷新成员列表
                if (state->successCount > 0) {
                    qDebug() << "所有成员上传完成，成功:" << state->successCount << "失败:" << state->failCount;
                    emit state->dlg->membersInvitedSuccess(state->groupId);
                }
                delete state;
            }
            
            reply->deleteLater();
            manager->deleteLater();
        });
    }
}

void FriendSelectDialog::clearLayout(QVBoxLayout* layout)
{
    if (!layout) return;
    
    // 清空复选框列表
    for (QCheckBox* checkbox : m_checkBoxes)
    {
        if (checkbox)
        {
            checkbox->deleteLater();
        }
    }
    m_checkBoxes.clear();
    m_friendInfoMap.clear(); // 清空好友信息映射

    QLayoutItem* child;
    while ((child = layout->takeAt(0)) != nullptr)
    {
        if (child->widget() && child->widget() != m_scrollWidget)
        {
            child->widget()->deleteLater();
        }
        delete child;
    }
    
    // 重新添加stretch
    layout->addStretch();
}

