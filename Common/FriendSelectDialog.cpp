#pragma execution_character_set("utf-8")
#include "FriendSelectDialog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QScrollArea>
#include <QWidget>

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
    
    if (!m_httpHandler) {
        QMessageBox::critical(this, "错误", "HTTP处理器未初始化！");
        return;
    }
    
    // 直接调用服务器接口邀请成员
    UserInfo userInfo = CommonInfo::GetData();
    QString url = "http://47.100.126.194:5000/groups/invite";
    
    // 构造成员数组
    QJsonArray membersArray;
    for (int i = 0; i < selectedTeacherIds.size(); i++)
    {
        QJsonObject memberObj;
        memberObj["unique_member_id"] = selectedTeacherIds[i];
        memberObj["member_name"] = selectedNames[i];
        memberObj["group_role"] = 1; // 1是普通成员，300是管理员角色，400是群主
        membersArray.append(memberObj);
    }
    
    // 构造请求JSON
    QJsonObject payload;
    payload["group_id"] = m_groupId;
    payload["members"] = membersArray;
    
    // 转换为JSON字符串
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "========== 邀请成员 - 服务器接口 ==========";
    qDebug() << "群组ID:" << m_groupId;
    qDebug() << "成员数量:" << membersArray.size();
    qDebug() << "请求JSON:" << QString::fromUtf8(jsonData);
    
    // 创建HTTP处理器用于接收响应
    TAHttpHandler* inviteHandler = new TAHttpHandler(this);
    connect(inviteHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
        qDebug() << "邀请成员服务器响应:" << responseString;
        
        // 解析响应
        QJsonDocument respDoc = QJsonDocument::fromJson(responseString.toUtf8());
        bool success = false;
        QString message = QString::fromUtf8(u8"邀请成员成功");
        
        if (respDoc.isObject()) {
            QJsonObject obj = respDoc.object();
            if (obj.contains("code")) {
                int code = obj.value("code").toInt(-1);
                success = (code == 0 || code == 200);
            }
            if (obj.contains("message") && obj.value("message").isString()) {
                message = obj.value("message").toString();
            }
        } else {
            // 非JSON响应但HTTP成功，视为成功
            success = true;
        }
        
        if (success) {
            QMessageBox::information(this, QString::fromUtf8(u8"邀请成功"), message);
            emit membersInvitedSuccess(m_groupId); // 发出信号通知刷新成员列表
            accept(); // 关闭对话框
        } else {
            QMessageBox::critical(this, QString::fromUtf8(u8"邀请失败"), message);
        }
        
        inviteHandler->deleteLater();
    });
    
    connect(inviteHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
        qDebug() << "邀请成员服务器错误:" << errResponseString;
        
        QString errorMsg = QString::fromUtf8(u8"邀请成员失败");
        QJsonDocument errDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
        if (errDoc.isObject()) {
            QJsonObject errObj = errDoc.object();
            if (errObj.contains("message") && errObj.value("message").isString()) {
                errorMsg = errObj.value("message").toString();
            }
        }
        
        QMessageBox::critical(this, QString::fromUtf8(u8"邀请失败"), errorMsg);
        inviteHandler->deleteLater();
    });
    
    // 发送POST请求
    inviteHandler->post(url, jsonData);
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

