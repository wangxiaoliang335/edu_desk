#pragma once

#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QComboBox>
#include <qjsonarray.h>
#include <QJsonParseError>
#include <qjsondocument.h>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "ImSDK/includes/TIMCloudCallback.h"

class GroupNotifyDialog : public QDialog
{
    Q_OBJECT
public:
    // 静态方法：提前注册群组系统消息回调（应在TIMInit之前调用）
    static void ensureCallbackRegistered()
    {
        static bool s_callbackRegistered = false;
        if (!s_callbackRegistered) {
            TIMSetGroupTipsEventCallback(staticGroupTipsCallback, nullptr);
            s_callbackRegistered = true;
            qDebug() << "GroupNotifyDialog: 群组系统消息回调已注册（TIMInit之前）";
        }
    }

    GroupNotifyDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        // 设置静态实例指针（用于全局回调发出信号）
        s_instance() = this;
        
        setWindowTitle("群通知");
        resize(750, 700);
        setStyleSheet("background-color: #f5f5f5; font-size: 14px;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // ===== 顶部标题栏 =====
        QHBoxLayout* titleLayout = new QHBoxLayout;
        QLabel* lblTitle = new QLabel("群通知");
        lblTitle->setStyleSheet("font-weight: bold; font-size: 20px;");
        QPushButton* btnFilter = new QPushButton("🔍");   // 可替换为图标
        QPushButton* btnDelete = new QPushButton("🗑");
        btnFilter->setFixedSize(30, 30);
        btnDelete->setFixedSize(30, 30);
        btnFilter->setFlat(true);
        btnDelete->setFlat(true);
        titleLayout->addWidget(lblTitle);
        titleLayout->addStretch();
        titleLayout->addWidget(btnFilter);
        titleLayout->addWidget(btnDelete);
        mainLayout->addLayout(titleLayout);

        // ===== 滚动区域 =====
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        QWidget* container = new QWidget;
        listLayout = new QVBoxLayout(container);
        listLayout->setSpacing(12);

        //// 添加两个示例条目
        //addItem(listLayout,
        //    "王小亮", "10:53",
        //    "申请加入", "大幅度反对法",
        //    "留言: 拿了");

        //addItem(listLayout,
        //    "王小亮", "10:51",
        //    "申请加入", "rttww",
        //    "留言: 可以吧");

        listLayout->addStretch();
        scroll->setWidget(container);
        mainLayout->addWidget(scroll);
        
        // 连接全局信号，接收群组系统消息
        connect(this, &GroupNotifyDialog::groupTipReceived, this, &GroupNotifyDialog::onGroupTipReceived);
    }

signals:
    // 群组系统消息信号（用于全局回调通知）
    void groupTipReceived(const QString& name, const QString& time, 
                         const QString& action, const QString& groupName, 
                         const QString& remark);

private:
    // 静态回调函数（在TIMInit之前注册）
    static void staticGroupTipsCallback(const char* json_group_tip_array, const void* user_data)
    {
        // 解析JSON数组
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(json_group_tip_array), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "群组系统消息JSON解析失败:" << parseError.errorString();
            return;
        }
        
        if (!doc.isArray()) {
            qDebug() << "群组系统消息数据不是数组";
            return;
        }
        
        QJsonArray tipArray = doc.array();
        for (const QJsonValue& value : tipArray) {
            if (!value.isObject()) continue;
            
            QJsonObject tipObj = value.toObject();
            
            // 获取消息类型
            int tipType = tipObj[kTIMGroupTipsElemTipType].toInt();
            QString groupId = tipObj[kTIMGroupTipsElemGroupId].toString();
            QString groupName = tipObj[kTIMGroupTipsElemGroupName].toString();
            QString opUser = tipObj[kTIMGroupTipsElemOpUser].toString();
            uint time = tipObj[kTIMGroupTipsElemTime].toInt();
            
            // 获取操作者信息
            QString opUserName = opUser;
            if (tipObj.contains(kTIMGroupTipsElemOpUserInfo)) {
                QJsonObject opUserInfo = tipObj[kTIMGroupTipsElemOpUserInfo].toObject();
                if (opUserInfo.contains("user_profile_nick_name")) {
                    opUserName = opUserInfo["user_profile_nick_name"].toString();
                }
            }
            
            // 获取被操作的用户列表
            QJsonArray userArray = tipObj[kTIMGroupTipsElemUserArray].toArray();
            QStringList userList;
            for (const QJsonValue& userValue : userArray) {
                userList.append(userValue.toString());
            }
            
            // 根据消息类型生成显示文本
            QString actionText;
            QString remark;
            
            switch (tipType) {
                case kTIMGroupTip_Invite:  // 邀请加入
                    if (userList.size() > 0) {
                        actionText = QString("邀请 %1 加入").arg(userList.join(", "));
                    } else {
                        actionText = "邀请加入";
                    }
                    break;
                case kTIMGroupTip_Quit:  // 退群
                    actionText = "退出群聊";
                    break;
                case kTIMGroupTip_Kick:  // 踢人
                    if (userList.size() > 0) {
                        actionText = QString("将 %1 移出群聊").arg(userList.join(", "));
                    } else {
                        actionText = "移出群成员";
                    }
                    break;
                case kTIMGroupTip_SetAdmin:  // 设置管理员
                    if (userList.size() > 0) {
                        actionText = QString("将 %1 设为管理员").arg(userList.join(", "));
                    } else {
                        actionText = "设置管理员";
                    }
                    break;
                case kTIMGroupTip_CancelAdmin:  // 取消管理员
                    if (userList.size() > 0) {
                        actionText = QString("取消 %1 的管理员身份").arg(userList.join(", "));
                    } else {
                        actionText = "取消管理员";
                    }
                    break;
                case kTIMGroupTip_GroupInfoChange:  // 群信息修改
                    actionText = "修改了群信息";
                    // 可以解析群资料变更信息
                    if (tipObj.contains(kTIMGroupTipsElemGroupChangeInfoArray)) {
                        QJsonArray changeArray = tipObj[kTIMGroupTipsElemGroupChangeInfoArray].toArray();
                        QStringList changeList;
                        for (const QJsonValue& changeValue : changeArray) {
                            if (changeValue.isObject()) {
                                QJsonObject changeObj = changeValue.toObject();
                                QString changeType = changeObj["group_tip_group_change_info_type"].toString();
                                QString changeValueStr = changeObj["group_tip_group_change_info_value"].toString();
                                if (changeType == "group_name") {
                                    changeList.append(QString("群名称: %1").arg(changeValueStr));
                                } else if (changeType == "introduction") {
                                    changeList.append("群简介");
                                } else if (changeType == "notification") {
                                    changeList.append("群公告");
                                }
                            }
                        }
                        if (!changeList.isEmpty()) {
                            remark = changeList.join(", ");
                        }
                    }
                    break;
                case kTIMGroupTip_MemberInfoChange:  // 群成员信息修改
                    actionText = "修改了群成员信息";
                    break;
                default:
                    actionText = "群组通知";
                    break;
            }
            
            // 转换时间戳为可读格式
            QDateTime dateTime = QDateTime::fromSecsSinceEpoch(time);
            QString timeStr = dateTime.toString("yyyy-MM-dd hh:mm:ss");
            
            // 发出全局信号，通知所有GroupNotifyDialog实例（使用QMetaObject::invokeMethod确保线程安全）
            if (s_instance()) {
                QMetaObject::invokeMethod(s_instance(), "onGroupTipReceived", Qt::QueuedConnection,
                    Q_ARG(QString, opUserName),
                    Q_ARG(QString, timeStr),
                    Q_ARG(QString, actionText),
                    Q_ARG(QString, groupName),
                    Q_ARG(QString, remark));
            }
        }
    }
    
    // 信号槽处理函数
    void onGroupTipReceived(const QString& name, const QString& time, 
                           const QString& action, const QString& groupName, 
                           const QString& remark)
    {
        addItem(listLayout, name, time, action, groupName, remark);
    }
    
    // 静态实例指针（用于发出信号）- 使用函数返回引用避免C++17要求
    static GroupNotifyDialog*& s_instance()
    {
        static GroupNotifyDialog* instance = nullptr;
        return instance;
    }

public:
    void InitData(QVector<QString> vecMsg = QVector<QString>())
    {
        if (m_bInit == true)
        {
            return;
        }

        for (int i = 0; i < vecMsg.size(); i++)
        {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(vecMsg[i].toUtf8(), &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON parse error:" << parseError.errorString();
            }
            else {
                if (doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj["data"].isArray())
                    {
                        // 4. 取出数组
                        QJsonArray arr = obj["data"].toArray();
                        // 5. 遍历数组
                        for (const QJsonValue& value : arr) {
                            if (value.isObject()) { // 每个元素是一个对象
                                QJsonObject obj = value.toObject();
                                int qSender_id = obj["sender_id"].toInt();
                                QString strSender_id = QString("%1").arg(qSender_id, 6, 10, QChar('0'));
                                QString SenderName = obj["sender_name"].toString();
                                int iContent_text = obj["content_text"].toInt();
                                QString updated_at = obj["updated_at"].toString();
                                int is_agreed = obj["is_agreed"].toInt();
                                QString content = obj["content"].toString();
                                QString GroupName = obj["group_name"].toString();
                                if (3 == iContent_text || 4 == iContent_text)
                                {
                                    addItem(listLayout, SenderName, updated_at, content,
                                        GroupName, "");
                                }
                            }
                        }
                    }
                }
            }
        }
        m_bInit = true;
    }

private slots:
    // 添加群组系统消息项（在主线程中调用）
    void addGroupTipItem(const QString& name, const QString& time, 
                        const QString& action, const QString& groupName, 
                        const QString& remark)
    {
        addItem(listLayout, name, time, action, groupName, remark);
    }

private:
    void addItem(QVBoxLayout* parent,
        const QString& name,
        const QString& time,
        const QString& action,
        const QString& groupName,
        const QString& remark)
    {
        QFrame* itemFrame = new QFrame;
        itemFrame->setStyleSheet("background-color: white; border-radius: 8px;");
        QVBoxLayout* outerLayout = new QVBoxLayout(itemFrame);

        // 第一行：头像 + 昵称 + 时间
        QHBoxLayout* topLayout = new QHBoxLayout;
        //QLabel* avatar = new QLabel;
        //avatar->setFixedSize(50, 50);
        //avatar->setStyleSheet("background-color: lightgray; border-radius: 25px;");
        QLabel* lblNameTime = new QLabel(QString("<b style='color:#0055cc'>%1</b> %2").arg(name, time));
        //topLayout->addWidget(avatar);
        topLayout->addWidget(lblNameTime);
        topLayout->addStretch();
        outerLayout->addLayout(topLayout);

        // 第二行：申请加入 + 群名
        QLabel* lblAction = new QLabel(QString("%1 <a href='#' style='color:#0055cc'>%2</a>").arg(action, groupName));
        lblAction->setTextFormat(Qt::RichText);
        lblAction->setOpenExternalLinks(false);
        outerLayout->addWidget(lblAction);

        // 留言
        QLabel* lblRemark = new QLabel(remark);
        lblRemark->setStyleSheet("color: gray;");
        outerLayout->addWidget(lblRemark);

        //// 操作按钮行
        //QHBoxLayout* btnLayout = new QHBoxLayout;
        //btnLayout->addStretch();
        //QPushButton* agreeBtn = new QPushButton("同意");
        //QComboBox* dropDown = new QComboBox;
        //dropDown->addItem("更多");
        //agreeBtn->setStyleSheet("background-color:#f5f5f5; border:1px solid lightgray; padding:4px;");
        //dropDown->setStyleSheet("background-color:#f5f5f5; border:1px solid lightgray;");
        //btnLayout->addWidget(agreeBtn);
        //btnLayout->addWidget(dropDown);
        //outerLayout->addLayout(btnLayout);

        parent->addWidget(itemFrame);
    }
    bool m_bInit = false;
    QVBoxLayout* listLayout = NULL;
};

