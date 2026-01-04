#pragma once
#pragma execution_character_set("utf-8")
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPoint>
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
    NormalGroupDialog(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

signals:
    void groupCreated(const QString& groupId); // 群组创建成功信号

private slots:
    void onCreateClicked();

private:
    void createNormalGroup(const QString& groupName, const UserInfo& userinfo);

private:
    QLineEdit* m_nameEdit = nullptr;
    TIMRestAPI* m_restAPI = nullptr;
    QPushButton* m_closeButton = nullptr;
    QLabel* m_titleLabel = nullptr;
    
    // 窗口样式相关
    bool m_dragging = false;
    QPoint m_dragStartPos;
    QColor m_backgroundColor = QColor(50, 50, 50);
    QColor m_borderColor = Qt::white;
    int m_borderWidth = 2;
    int m_radius = 6;
};

