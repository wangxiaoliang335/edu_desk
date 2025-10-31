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
#include "CommonInfo.h"
#include "CourseDialog.h"

class ClassTeacherDialog;
class ClassTeacherDelDialog;
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
public:
    QGroupInfo(QWidget* parent = nullptr);
    ~QGroupInfo();

public:
    void initData(QString groupName, QString groupNumberId);
    void InitGroupMember(QVector<GroupMemberInfo> groupMemberInfo);
private:
    QString m_groupName;
    QString m_groupNumberId;
    QVector<GroupMemberInfo> m_groupMemberInfo;
    QHBoxLayout* circlesLayout = NULL;
    ClassTeacherDialog* m_classTeacherDlg = NULL;
    ClassTeacherDelDialog* m_classTeacherDelDlg = NULL;
    CourseDialog* m_courseDlg;
};


