#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>
#include <QResizeEvent>

// 自定义消息对话框类
class CustomMessageDialog : public QDialog
{
public:
    explicit CustomMessageDialog(QWidget* parent = nullptr, const QString& title = "", const QString& message = "")
        : QDialog(parent), m_dragging(false), m_cornerRadius(16)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        resize(400, 200);
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(16);
        
        // 标题
        QLabel* titleLabel = new QLabel(title, this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold; padding: 8px 0px;");
        mainLayout->addWidget(titleLabel);
        
        // 消息内容
        QLabel* messageLabel = new QLabel(message, this);
        messageLabel->setAlignment(Qt::AlignCenter);
        messageLabel->setWordWrap(true);
        messageLabel->setStyleSheet("color: white; font-size: 14px; padding: 8px;");
        mainLayout->addWidget(messageLabel);
        
        // 确定按钮
        QPushButton* okButton = new QPushButton(QString::fromUtf8(u8"确定"), this);
        okButton->setStyleSheet("background-color: #1976d2; color: white; padding: 8px 16px; border-radius: 4px; font-size: 14px;");
        okButton->setFixedHeight(36);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        buttonLayout->addWidget(okButton);
        buttonLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
        
        setStyleSheet("QDialog { background-color: #323232; border-radius: 16px; }");
        updateMask();
    }
    
    static int showMessage(QWidget* parent, const QString& title, const QString& message)
    {
        CustomMessageDialog dialog(parent, title, message);
        return dialog.exec();
    }
    
protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect rect(0, 0, width(), height());
        QPainterPath path;
        path.addRoundedRect(rect, m_cornerRadius, m_cornerRadius);
        p.fillPath(path, QBrush(QColor(50, 50, 50)));
        QPen pen(Qt::white, 2);
        p.strokePath(path, pen);
    }
    
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
        QDialog::mousePressEvent(event);
    }
    
    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
        QDialog::mouseMoveEvent(event);
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
        QDialog::mouseReleaseEvent(event);
    }
    
    void resizeEvent(QResizeEvent* event) override
    {
        QDialog::resizeEvent(event);
        updateMask();
    }
    
private:
    void updateMask()
    {
        QPainterPath path;
        path.addRoundedRect(0, 0, width(), height(), m_cornerRadius, m_cornerRadius);
        QRegion region(path.toFillPolygon().toPolygon());
        setMask(region);
    }
    
    bool m_dragging;
    QPoint m_dragStartPos;
    int m_cornerRadius;
};
