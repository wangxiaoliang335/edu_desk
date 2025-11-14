#pragma once
#pragma execution_character_set("utf-8")
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "TIMRestAPI.h"
#include "GenerateTestUserSig.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QPointer>
#include <QMetaObject>

class NormalGroupDialog : public QDialog
{
    Q_OBJECT
public:
    NormalGroupDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("创建普通群"));
        resize(400, 200);
        setStyleSheet("background-color:#555555; font-size:14px;");

        m_httpHandler = new TAHttpHandler(this);
        m_restAPI = new TIMRestAPI(this);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // 群组名称输入
        QLabel* nameLabel = new QLabel(QStringLiteral("群组名称:"), this);
        nameLabel->setStyleSheet("color: white;");
        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setPlaceholderText(QStringLiteral("请输入群组名称"));
        m_nameEdit->setStyleSheet("background-color: white; padding: 8px;");

        // 按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        QPushButton* btnCreate = new QPushButton(QStringLiteral("创建"), this);
        QPushButton* btnCancel = new QPushButton(QStringLiteral("取消"), this);
        btnCreate->setStyleSheet("background-color: green; color: white; padding: 8px;");
        btnCancel->setStyleSheet("background-color: gray; color: white; padding: 8px;");

        mainLayout->addWidget(nameLabel);
        mainLayout->addWidget(m_nameEdit);
        mainLayout->addLayout(buttonLayout);
        buttonLayout->addStretch();
        buttonLayout->addWidget(btnCreate);
        buttonLayout->addWidget(btnCancel);

        connect(btnCreate, &QPushButton::clicked, this, &NormalGroupDialog::onCreateClicked);
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    }

signals:
    void groupCreated(const QString& groupId); // 群组创建成功信号

private slots:
    void onCreateClicked()
    {
        QString groupName = m_nameEdit->text().trimmed();
        if (groupName.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("群组名称不能为空！"));
            return;
        }

        UserInfo userinfo = CommonInfo::GetData();
        createNormalGroup(groupName, userinfo);
    }

private:
    void createNormalGroup(const QString& groupName, const UserInfo& userinfo)
    {
        if (!m_restAPI) {
            QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("REST API未初始化！"));
            return;
        }

        // 在使用REST API前设置管理员账号信息
        std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
        if (!adminUserId.empty()) {
            std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
            m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
        } else {
            QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("管理员账号未设置！\n请确保用户已登录且CommonInfo已初始化用户信息。"));
            return;
        }

        // 构造初始成员数组（只有创建者）
        QJsonArray memberArray;
        QString creatorId = userinfo.teacher_unique_id;
        QJsonObject ownerMemberInfo;
        ownerMemberInfo["Member_Account"] = creatorId;
        ownerMemberInfo["Role"] = "Admin";
        memberArray.append(ownerMemberInfo);

        // 创建回调数据结构
        struct CreateGroupCallbackData {
            NormalGroupDialog* dlg;
            QString groupName;
            UserInfo userInfo;
        };

        CreateGroupCallbackData* callbackData = new CreateGroupCallbackData;
        callbackData->dlg = this;
        callbackData->groupName = groupName;
        callbackData->userInfo = userinfo;

        // 调用REST API创建群组
        m_restAPI->createGroup(groupName, "Meeting", memberArray,
            [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg = QString(QStringLiteral("创建群组失败: %1 (错误码: %2)")).arg(errorDesc).arg(errorCode);
                    qDebug() << errorMsg;
                    QMessageBox::critical(callbackData->dlg, QStringLiteral("创建群组失败"), errorMsg);
                    delete callbackData;
                    return;
                }

                // 创建群组成功，解析返回的群组ID
                QString groupId = result["GroupId"].toString();
                if (groupId.isEmpty()) {
                    QJsonObject groupInfo = result["GroupInfo"].toObject();
                    groupId = groupInfo["GroupId"].toString();
                }

                qDebug() << "创建普通群成功，群组ID:" << groupId;

                if (!groupId.isEmpty()) {
                    // 上传群组信息到服务器（普通群，is_class_group=0）
                    if (callbackData->dlg && callbackData->dlg->m_httpHandler) {
                        callbackData->dlg->uploadGroupInfoToServer(groupId, callbackData->groupName, callbackData->userInfo);
                    }

                    QMessageBox::information(callbackData->dlg, QStringLiteral("创建群组成功"),
                        QString(QStringLiteral("群组创建成功！\n群组ID: %1")).arg(groupId));

                    // 发出群组创建成功信号
                    emit callbackData->dlg->groupCreated(groupId);
                    callbackData->dlg->accept();
                } else {
                    qWarning() << "创建群组成功但未获取到群组ID";
                    QMessageBox::warning(callbackData->dlg, QStringLiteral("警告"), QStringLiteral("创建群组成功但未获取到群组ID"));
                }

                delete callbackData;
            });
    }

    // 上传群组信息到服务器（普通群）
    void uploadGroupInfoToServer(const QString& groupId, const QString& groupName, const UserInfo& userinfo)
    {
        QJsonObject groupObj;

        // 群组基础信息
        groupObj["group_id"] = groupId;
        groupObj["group_name"] = groupName;
        groupObj["group_type"] = 0; // 公开群（Public），支持设置管理员
        groupObj["face_url"] = "";
        groupObj["info_seq"] = 0;
        groupObj["latest_seq"] = 0;
        groupObj["is_shutup_all"] = false;

        // 群组详细信息
        groupObj["detail_group_id"] = groupId;
        groupObj["detail_group_name"] = groupName;
        groupObj["detail_group_type"] = 0; // 公开群（Public），支持设置管理员
        groupObj["detail_face_url"] = "";
        groupObj["create_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        groupObj["detail_info_seq"] = 0;
        groupObj["introduction"] = QString(QStringLiteral("普通群：%1")).arg(groupName);
        groupObj["notification"] = QString(QStringLiteral("欢迎加入%1")).arg(groupName);
        groupObj["last_info_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        groupObj["last_msg_time"] = 0;
        groupObj["next_msg_seq"] = 0;
        groupObj["member_num"] = 1;
        groupObj["max_member_num"] = 2000;
        groupObj["online_member_num"] = 0;
        groupObj["owner_identifier"] = userinfo.teacher_unique_id;
        groupObj["add_option"] = 0; // kTIMGroupAddOpt_Any
        groupObj["visible"] = 2;
        groupObj["searchable"] = 2;
        groupObj["detail_is_shutup_all"] = false;
        groupObj["classid"] = ""; // 普通群没有班级ID
        groupObj["schoolid"] = userinfo.schoolId;
        groupObj["is_class_group"] = 0; // 普通群，设置为0

        // 用户在该群组中的信息
        QJsonObject memberInfo;
        memberInfo["user_id"] = userinfo.teacher_unique_id;
        memberInfo["readed_seq"] = 0;
        memberInfo["msg_flag"] = 0;
        memberInfo["join_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        memberInfo["self_role"] = 400; // 群主
        memberInfo["self_msg_flag"] = 0;
        memberInfo["unread_num"] = 0;
        memberInfo["user_name"] = userinfo.strName;

        groupObj["member_info"] = memberInfo;

        QJsonArray groupsArray;
        groupsArray.append(groupObj);

        QJsonObject uploadData;
        uploadData["user_id"] = userinfo.teacher_unique_id;
        uploadData["groups"] = groupsArray;

        QJsonDocument uploadDoc(uploadData);
        QByteArray jsonData = uploadDoc.toJson(QJsonDocument::Compact);

        QString url = "http://47.100.126.194:5000/groups/sync";

        QPointer<NormalGroupDialog> self = this;
        QMetaObject::Connection* conn = new QMetaObject::Connection;
        *conn = connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            if (responseString.contains(groupId) || responseString.contains("\"code\":200") || responseString.contains("\"code\": 200")) {
                qDebug() << "普通群信息上传成功，响应:" << responseString;

                if (conn && *conn) {
                    disconnect(*conn);
                    delete conn;
                }
            }
        }, Qt::UniqueConnection);

        m_httpHandler->post(url, jsonData);
        qDebug() << "上传普通群信息到服务器，群组ID:" << groupId;
    }

private:
    QLineEdit* m_nameEdit = nullptr;
    TAHttpHandler* m_httpHandler = nullptr;
    TIMRestAPI* m_restAPI = nullptr;
};

