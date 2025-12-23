#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include "CommonInfo.h"
#include "TaQTWebSocket.h"

// 通知消息对话框（类似图片中的样式）
class NotificationMessageDialog : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationMessageDialog(const QString& className, const QString& classId, 
                                       const QString& groupId = QString(), 
                                       const QString& groupName = QString(), 
                                       QWidget* parent = nullptr);
    ~NotificationMessageDialog();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onCancelClicked();
    void onSendClicked();

private:
    void setupUI();
    void sendNotification();

private:
    QString m_className;
    QString m_classId;
    QString m_groupId;
    QString m_groupName;
    
    QLabel* m_titleLabel;
    QPushButton* m_closeButton;
    QTextEdit* m_textEdit;
    QPushButton* m_cancelButton;
    QPushButton* m_sendButton;
    
    bool m_dragging;
    QPoint m_dragStartPos;
};

