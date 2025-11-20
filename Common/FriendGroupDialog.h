#pragma once

#include <QApplication>
#include <QDialog>
#include <QStackedWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QFrame>
#include <QMouseEvent>
#include <QStyle>
#include <QDir>
#include <qpainterpath.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "FriendNotifyDialog.h"
#include "TACAddGroupWidget.h"
#include "GroupNotifyDialog.h"
#include "TAHttpHandler.h"
#include "ImSDK/includes/TIMCloud.h"
#include "CommonInfo.h"
#include <QMap>

class ScheduleDialog;

class RowItem : public QFrame {
    Q_OBJECT
public:
    explicit RowItem(const QString& text, QWidget* parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* e) override;
};

class FriendGroupDialog : public QDialog
{
    Q_OBJECT
public:
    FriendGroupDialog(QWidget* parent, TaQTWebSocket* pWs);

    // 帮助函数：生成一行左右两个按钮布局（相同颜色）
    static QHBoxLayout* makeRowBtn(const QString& leftText, const QString& rightText, const QString& bgColor, const QString& fgColor);

    // 辅助函数：为 ScheduleDialog 建立群聊退出信号连接
    void connectGroupLeftSignal(ScheduleDialog* scheduleDlg, const QString& groupId);

    // 帮助函数：生成一行两个不同用途的按钮（如头像+昵称）
    QHBoxLayout* makePairBtn(const QString& leftText, const QString& rightText, const QString& bgColor, const QString& fgColor, QString unique_group_id, QString classid, bool iGroupOwner, bool isClassGroup = true);

    void InitData();

    void GetGroupJoinedList(); // 已加入群列表

    void InitWebSocket();

    void setTitleName(const QString& name);

    void visibleCloseButton(bool val);

    void setBackgroundColor(const QColor& color);

    void setBorderColor(const QColor& color);

    void setBorderWidth(int val);

    void setRadius(int val);

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void leaveEvent(QEvent* event) override;

    void enterEvent(QEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

public:
    TACAddGroupWidget1* addGroupWidget = NULL;
    FriendNotifyDialog* friendNotifyDlg = NULL;
    GroupNotifyDialog* grpNotifyDlg = NULL;

private slots:
    void onWebSocketMessage(const QString& msg);

private:
    bool m_dragging;
    QPoint m_dragStartPos;
    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    bool m_visibleCloseButton;
    QPushButton* closeButton = NULL;
    QLabel* pLabel = NULL;
    TAHttpHandler* m_httpHandler = NULL;
    QVBoxLayout* fLayout = NULL;
    QVBoxLayout* gLayout = NULL;
    QVBoxLayout* gAdminLayout = NULL; // 班级群-管理布局
    QVBoxLayout* gJoinLayout = NULL; // 班级群-加入布局
    QVBoxLayout* gNormalAdminLayout = NULL; // 普通群-管理布局
    QVBoxLayout* gNormalJoinLayout = NULL; // 普通群-加入布局
    TaQTWebSocket* m_pWs = NULL;
    QMap<QString, ScheduleDialog*> m_scheduleDlg;
    QList<Notification> notifications;
    QSet<QString> m_setClassId;
};
