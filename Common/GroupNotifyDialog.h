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
#include <QList>
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
        deliverPendingTips();
    }

signals:
    // 群组系统消息信号（用于全局回调通知）
    void groupTipReceived(const QString& name, const QString& time, 
                         const QString& action, const QString& groupName, 
                         const QString& remark);
    
    // 新成员入群信号（用于自动刷新成员列表）
    void newMemberJoined(const QString& groupId, const QStringList& memberIds);

public:
    // 静态方法：获取实例（用于外部连接信号）
    static GroupNotifyDialog* instance()
    {
        return s_instance();
    }

private:
    // 静态回调函数（在TIMInit之前注册）
    static void staticGroupTipsCallback(const char* json_group_tip_array, const void* user_data)
    {
        qDebug() << "========== 收到群组系统消息回调 ==========";
        qDebug() << "原始JSON数据:" << QString::fromUtf8(json_group_tip_array);
        
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
        
        handleGroupTipsArray(doc.array());
        qDebug() << "==========================================";
    }

public:
    static void processIncomingGroupTips(const QJsonArray& tipArray)
    {
        handleGroupTipsArray(tipArray);
    }
    
public slots:
    // 信号槽处理函数
    void onGroupTipReceived(const QString& name, const QString& time, 
                           const QString& action, const QString& groupName, 
                           const QString& remark)
    {
        qDebug() << "========== GroupNotifyDialog::onGroupTipReceived ==========";
        qDebug() << "名称:" << name;
        qDebug() << "时间:" << time;
        qDebug() << "操作:" << action;
        qDebug() << "群组名称:" << groupName;
        qDebug() << "备注:" << remark;
        qDebug() << "正在添加通知项到列表";
        
        if (!listLayout) {
            return;
        }
        QFrame* itemFrame = createItem(name, time, action, groupName, remark);
        int insertIndex = listLayout->count() - 1;
        if (insertIndex < 0) insertIndex = 0;
        listLayout->insertWidget(insertIndex, itemFrame);
        
        qDebug() << "通知项已添加";
        qDebug() << "==========================================";
    }
    
    // 新成员入群处理函数
    void onNewMemberJoined(const QString& groupId, const QStringList& memberIds)
    {
        qDebug() << "========== GroupNotifyDialog::onNewMemberJoined ==========";
        qDebug() << "群组ID:" << groupId;
        qDebug() << "新成员ID列表:" << memberIds.join(", ");
        qDebug() << "发出新成员入群信号，通知相关窗口刷新成员列表";
        
        // 发出信号，通知其他窗口刷新成员列表
        emit newMemberJoined(groupId, memberIds);
        
        qDebug() << "==========================================";
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
                                    if (listLayout) {
                                        QFrame* itemFrame = createItem(SenderName, updated_at, content, GroupName, "");
                                        int insertIndex = listLayout->count() - 1;
                                        if (insertIndex < 0) insertIndex = 0;
                                        listLayout->insertWidget(insertIndex, itemFrame);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        m_bInit = true;
    }

private:
    QFrame* createItem(const QString& name,
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

        return itemFrame;
    }
    bool m_bInit = false;
    QVBoxLayout* listLayout = NULL;

    static void handleGroupTipsArray(const QJsonArray& tipArray)
    {
        qDebug() << "群组系统消息数量:" << tipArray.size();
        for (const QJsonValue& value : tipArray) {
            if (!value.isObject()) continue;
            QJsonObject tipObj = value.toObject();
            int tipType = tipObj[kTIMGroupTipsElemTipType].toInt();
            QString groupId = tipObj[kTIMGroupTipsElemGroupId].toString();
            QString groupName = tipObj[kTIMGroupTipsElemGroupName].toString();
            QString opUser = tipObj[kTIMGroupTipsElemOpUser].toString();
            uint time = tipObj[kTIMGroupTipsElemTime].toInt();

            qDebug() << "消息类型:" << tipType << "(kTIMGroupTip_Invite=" << kTIMGroupTip_Invite << ")";
            qDebug() << "群组ID:" << groupId;
            qDebug() << "群组名称:" << groupName;
            qDebug() << "操作者:" << opUser;

            QString opUserName = opUser;
            if (tipObj.contains(kTIMGroupTipsElemOpUserInfo)) {
                QJsonObject opUserInfo = tipObj[kTIMGroupTipsElemOpUserInfo].toObject();
                if (opUserInfo.contains("user_profile_nick_name")) {
                    opUserName = opUserInfo["user_profile_nick_name"].toString();
                }
            }

            QJsonArray userArray = tipObj[kTIMGroupTipsElemUserArray].toArray();
            QStringList userList;
            for (const QJsonValue& userValue : userArray) {
                userList.append(userValue.toString());
            }

            QString actionText;
            QString remark;

            switch (tipType) {
                case kTIMGroupTip_Invite:
                    if (!userList.isEmpty()) {
                        actionText = QString("邀请 %1 加入").arg(userList.join(", "));
                        if (s_instance()) {
                            qDebug() << "检测到新成员入群，群组ID:" << groupId << "，新成员:" << userList.join(", ");
                            QMetaObject::invokeMethod(s_instance(), "onNewMemberJoined", Qt::QueuedConnection,
                                Q_ARG(QString, groupId),
                                Q_ARG(QStringList, userList));
                        }
                    } else {
                        actionText = "邀请加入";
                    }
                    break;
                case kTIMGroupTip_Quit:
                    actionText = "退出群聊";
                    break;
                case kTIMGroupTip_Kick:
                    actionText = userList.isEmpty()
                        ? "移出群成员"
                        : QString("将 %1 移出群聊").arg(userList.join(", "));
                    break;
                case kTIMGroupTip_SetAdmin:
                    actionText = userList.isEmpty()
                        ? "设置管理员"
                        : QString("将 %1 设为管理员").arg(userList.join(", "));
                    break;
                case kTIMGroupTip_CancelAdmin:
                    actionText = userList.isEmpty()
                        ? "取消管理员"
                        : QString("取消 %1 的管理员身份").arg(userList.join(", "));
                    break;
                case kTIMGroupTip_GroupInfoChange:
                    actionText = "修改了群信息";
                    if (tipObj.contains(kTIMGroupTipsElemGroupChangeInfoArray)) {
                        QJsonArray changeArray = tipObj[kTIMGroupTipsElemGroupChangeInfoArray].toArray();
                        QStringList changeList;
                        for (const QJsonValue& changeValue : changeArray) {
                            if (!changeValue.isObject()) continue;
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
                        if (!changeList.isEmpty()) {
                            remark = changeList.join(", ");
                        }
                    }
                    break;
                case kTIMGroupTip_MemberInfoChange:
                    actionText = "修改了群成员信息";
                    break;
                default:
                    actionText = "群组通知";
                    break;
            }

            QDateTime dateTime = QDateTime::fromSecsSinceEpoch(time);
            QString timeStr = dateTime.toString("yyyy-MM-dd hh:mm:ss");

            qDebug() << "操作文本:" << actionText;
            qDebug() << "备注:" << remark;
            qDebug() << "GroupNotifyDialog实例是否存在:" << (s_instance() != nullptr);

            GroupTipCache cacheItem{ opUserName, timeStr, actionText, groupName, remark, groupId, tipType, userList };
            if (s_instance()) {
                qDebug() << "正在发送信号到GroupNotifyDialog实例";
                QMetaObject::invokeMethod(s_instance(), "onGroupTipReceived", Qt::QueuedConnection,
                    Q_ARG(QString, cacheItem.opUserName),
                    Q_ARG(QString, cacheItem.timeStr),
                    Q_ARG(QString, cacheItem.actionText),
                    Q_ARG(QString, cacheItem.groupName),
                    Q_ARG(QString, cacheItem.remark));
                if (tipType == kTIMGroupTip_Invite && !userList.isEmpty()) {
                    QMetaObject::invokeMethod(s_instance(), "onNewMemberJoined", Qt::QueuedConnection,
                        Q_ARG(QString, groupId),
                        Q_ARG(QStringList, userList));
                }
            } else {
                pendingGroupTips().append(cacheItem);
            }
        }
    }

    struct GroupTipCache {
        QString opUserName;
        QString timeStr;
        QString actionText;
        QString groupName;
        QString remark;
        QString groupId;
        int tipType = 0;
        QStringList memberList;
    };

    static QList<GroupTipCache>& pendingGroupTips()
    {
        static QList<GroupTipCache> cache;
        return cache;
    }

    void deliverPendingTips()
    {
        auto& cache = pendingGroupTips();
        for (const auto& tip : cache) {
            onGroupTipReceived(tip.opUserName, tip.timeStr, tip.actionText, tip.groupName, tip.remark);
            if (tip.tipType == kTIMGroupTip_Invite && !tip.memberList.isEmpty()) {
                onNewMemberJoined(tip.groupId, tip.memberList);
            }
        }
        cache.clear();
    }
};

