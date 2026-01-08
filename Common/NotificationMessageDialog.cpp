#pragma execution_character_set("utf-8")
#include "NotificationMessageDialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QApplication>

NotificationMessageDialog::NotificationMessageDialog(const QString& className, const QString& classId,
                                                     const QString& groupId, const QString& groupName,
                                                     QWidget* parent)
    : QWidget(parent)
    , m_className(className)
    , m_classId(classId)
    , m_groupId(groupId)
    , m_groupName(groupName)
    , m_dragging(false)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    
    setupUI();
    
    // 设置窗口大小
    setFixedSize(500, 400);
    
    // 居中显示
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
}

NotificationMessageDialog::~NotificationMessageDialog()
{
}

void NotificationMessageDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 标题栏
    QWidget* titleBar = new QWidget(this);
    titleBar->setFixedHeight(50);
    titleBar->setStyleSheet("background-color: #2b2b2b; border-top-left-radius: 8px; border-top-right-radius: 8px; border: none;");
    
    QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);
    titleLayout->setSpacing(10);
    
    // 关闭按钮
    m_closeButton = new QPushButton("×", titleBar);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "border: none;"
        "color: #ffffff;"
        "background: rgba(255,255,255,0.12);"
        "border-radius: 12px;"
        "font-weight: bold;"
        "font-size: 16px;"
        "}"
        "QPushButton:hover {"
        "background: rgba(255,0,0,0.35);"
        "}"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &NotificationMessageDialog::close);
    
    // 标题文本
    QString titleText = QString::fromUtf8(u8"通知");
    m_titleLabel = new QLabel(titleText, titleBar);
    m_titleLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold; background: transparent; border: none;");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    titleLayout->addWidget(m_closeButton);
    titleLayout->addWidget(m_titleLabel, 1);
    titleLayout->addStretch();
    
    mainLayout->addWidget(titleBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background-color: #2b2b2b;");
    
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(15);
    
    // 文本输入框
    m_textEdit = new QTextEdit(contentWidget);
    m_textEdit->setPlaceholderText(QString::fromUtf8(u8"请输入需要发送的文本消息"));
    m_textEdit->setStyleSheet(
        "QTextEdit {"
        "background-color: #1e1e1e;"
        "border: 1px solid #404040;"
        "border-radius: 6px;"
        "color: #ffffff;"
        "font-size: 13px;"
        "padding: 10px;"
        "min-height: 200px;"
        "}"
        "QTextEdit:focus {"
        "border: 1px solid #4169E1;"
        "}"
    );
    
    contentLayout->addWidget(m_textEdit, 1);
    
    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(15);
    buttonLayout->addStretch();
    
    // 取消按钮
    m_cancelButton = new QPushButton(QString::fromUtf8(u8"取消"), contentWidget);
    m_cancelButton->setFixedSize(80, 35);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "background-color: #2D2E2D;"
        "color: #ffffff;"
        "border: none;"
        "border-radius: 6px;"
        "font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "background-color: #3D3E3D;"
        "}"
        "QPushButton:pressed {"
        "background-color: #1D1E1D;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &NotificationMessageDialog::onCancelClicked);
    
    // 发送按钮
    m_sendButton = new QPushButton(QString::fromUtf8(u8"发送"), contentWidget);
    m_sendButton->setFixedSize(80, 35);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "background-color: #4169E1;"
        "color: #ffffff;"
        "border: none;"
        "border-radius: 6px;"
        "font-size: 13px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #5B7FD8;"
        "}"
        "QPushButton:pressed {"
        "background-color: #3357C7;"
        "}"
    );
    connect(m_sendButton, &QPushButton::clicked, this, &NotificationMessageDialog::onSendClicked);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_sendButton);
    contentLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(contentWidget);
    
    // 底部圆角
    contentWidget->setStyleSheet(contentWidget->styleSheet() + "border-bottom-left-radius: 8px; border-bottom-right-radius: 8px;");
}

void NotificationMessageDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制圆角矩形背景
    QRect rect = this->rect();
    QPainterPath path;
    path.addRoundedRect(rect, 8, 8);
    
    painter.fillPath(path, QColor(43, 43, 43));
    painter.strokePath(path, QPen(QColor(120, 120, 120), 1));
}

void NotificationMessageDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void NotificationMessageDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
        event->accept();
    }
}

void NotificationMessageDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

void NotificationMessageDialog::onCancelClicked()
{
    close();
}

void NotificationMessageDialog::onSendClicked()
{
    QString content = m_textEdit->toPlainText().trimmed();
    if (content.isEmpty()) {
        // 内容为空时不发送，直接返回
        return;
    }
    
    sendNotification();
    close();
}

void NotificationMessageDialog::sendNotification()
{
    QString content = m_textEdit->toPlainText().trimmed();
    if (content.isEmpty()) {
        return;
    }
    
    // 获取用户信息
    UserInfo userInfo = CommonInfo::GetData();
    
    // 构建通知消息 JSON（方式一：纯 JSON 格式，推荐）
    QJsonObject notificationObj;
    notificationObj["type"] = "notification";
    notificationObj["class_id"] = m_classId;  // 必须：班级代码
    notificationObj["content"] = content;
    notificationObj["content_text"] = QString::fromUtf8(u8"通知");
    notificationObj["sender_name"] = userInfo.strName;
    
    // receiver_id 设置为班级ID（接收者是班级端）
    notificationObj["receiver_id"] = m_classId;
    
    // 可选字段
    if (!userInfo.teacher_unique_id.isEmpty()) {
        notificationObj["sender_id"] = userInfo.teacher_unique_id;
    }
    
    if (!m_groupId.isEmpty()) {
        notificationObj["group_id"] = m_groupId;
    }
    
    if (!m_groupName.isEmpty()) {
        notificationObj["group_name"] = m_groupName;
    }
    
    // 序列化为 JSON 字符串
    QJsonDocument doc(notificationObj);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    
    // 通过 WebSocket 发送
    TaQTWebSocket::sendPrivateMessage(jsonString);
    
    qDebug() << "发送通知消息:" << jsonString;
}

