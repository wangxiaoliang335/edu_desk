#pragma once

#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QSpacerItem>
#include <QDebug>
#include <QCheckBox>
#include <QGroupBox>
#include <QMenu>
#include <QAction>
#include <QCheckBox>
#include <QMouseEvent>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include "CommonInfo.h"
#include "CourseDialog.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"
#include "ImSDK/includes/TIMCloudCallback.h"

class ClassTeacherDialog;
class ClassTeacherDelDialog;
class FriendSelectDialog;
class FriendButton : public QPushButton {
    Q_OBJECT
public:
    FriendButton(const QString& text = "", QWidget* parent = nullptr) : QPushButton(text, parent) {
        setFixedSize(50, 50);
        setStyleSheet("background-color:blue; border-radius:25px; color:white; font-weight:bold;");
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &FriendButton::customContextMenuRequested, this, &FriendButton::showContextMenu);
    }

signals:
    void setLeaderRequested();
    void cancelLeaderRequested();

private slots:
    void showContextMenu(const QPoint& pos) {
        QMenu menu;
        QAction* actSet = menu.addAction("设为班主任");
        QAction* actCancel = menu.addAction("取消班主任");

        QAction* chosen = menu.exec(mapToGlobal(pos));
        if (chosen == actSet) {
            emit setLeaderRequested();
        }
        else if (chosen == actCancel) {
            emit cancelLeaderRequested();
        }
    }
};

class QGroupInfo : public QDialog {
    Q_OBJECT
public:
    QGroupInfo(QWidget* parent = nullptr);
    ~QGroupInfo();

public:
    void initData(QString groupName, QString groupNumberId);
    void InitGroupMember(QString group_id, QVector<GroupMemberInfo> groupMemberInfo);
    void InitGroupMember();
    QVector<GroupMemberInfo> getGroupMemberInfo() const { return m_groupMemberInfo; } // 获取当前成员列表
    
signals:
    void memberLeftGroup(const QString& groupId, const QString& leftUserId); // 成员退出群聊信号，传递退出的用户ID
    void groupDismissed(const QString& groupId); // 群聊解散信号，通知父窗口刷新群列表

private:
    void updateButtonStates(); // 根据当前用户角色更新按钮状态
    void onExitGroupClicked(); // 退出群聊按钮点击处理
    void onDismissGroupClicked(); // 解散群聊按钮点击处理
    void sendExitGroupRequestToServer(const QString& groupId, const QString& userId, const QString& leftUserId); // 发送退出群聊请求到服务器
    void sendDismissGroupRequestToServer(const QString& groupId, const QString& userId, void* callbackData = nullptr); // 发送解散群聊请求到服务器
    QString m_groupName;
    QString m_groupNumberId;
    QVector<GroupMemberInfo> m_groupMemberInfo;
    QHBoxLayout* circlesLayout = NULL;
    FriendSelectDialog* m_friendSelectDlg = NULL;
    //ClassTeacherDelDialog* m_classTeacherDelDlg = NULL;
    CourseDialog* m_courseDlg;
    QPushButton* m_btnDismiss = nullptr; // 解散群聊按钮
    QPushButton* m_btnExit = nullptr; // 退出群聊按钮
};


