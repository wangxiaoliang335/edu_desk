#pragma once

#include "mainwindow.h"
#include <QMouseEvent>
#include <QPoint>
#include <QTimer>
#include <QVBoxLayout>
#include <QLayoutItem>
#include <QShowEvent>
#include <QResizeEvent>
#include <QEvent>

// 对外统一的教师课程表窗口名（不使用 Q_OBJECT，因此不需要单独 moc）
class TACTeacherCourseScheduleWindow : public MainWindow
{
public:
    explicit TACTeacherCourseScheduleWindow(QWidget* parent = nullptr)
        : MainWindow(parent)
    {
        setObjectName(QStringLiteral("TACTeacherCourseScheduleWindow"));
        setWindowTitle(QString::fromUtf8(u8"我的课表"));
        
        // 立即设置窗口标志，确保无标题栏
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        
        // 设置窗口大小
        resize(1000, 700);
        setMinimumSize(800, 600);
        
        // 设置背景颜色和文本颜色，与TACMainDialog和DutyRosterDialog一致的深色主题
        // 应用到整个窗口和所有子控件
        setStyleSheet(
            "TACTeacherCourseScheduleWindow { "
            "background-color: #282A2B; "
            "color: #ffffff; "
            "} "
            "QWidget { "
            "background-color: #282A2B; "
            "color: #ffffff; "
            "} "
            "QMainWindow { "
            "background-color: #282A2B; "
            "}"
        );
        
        // 延迟执行，确保MainWindow的setupUi已经完成
        QTimer::singleShot(50, this, [this]() {
            // 再次确保窗口标志
            setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
            
            // 确保工具栏可见
            ensureToolbarVisible();
            applyToolbarStyle();
            
            // 确保窗口和中央控件都应用了样式
            QWidget* central = centralWidget();
            if (central) {
                central->setStyleSheet("background-color: #282A2B; color: #ffffff;");
            }
        });
        
        // 安装事件过滤器，拦截所有可能修改窗口标志的事件
        installEventFilter(this);
        
        // 创建定时器，定期检查并确保工具栏可见（包括极简模式）
        m_toolbarCheckTimer = new QTimer(this);
        m_toolbarCheckTimer->setInterval(100); // 每100ms检查一次
        m_toolbarCheckTimer->setSingleShot(false);
        connect(m_toolbarCheckTimer, &QTimer::timeout, this, [this]() {
            ensureToolbarVisible();
        });
        m_toolbarCheckTimer->start();
        
        // 创建定时器，更频繁地检查窗口标志，确保始终无标题栏
        m_windowFlagsTimer = new QTimer(this);
        m_windowFlagsTimer->setInterval(50); // 每50ms检查一次，更频繁
        m_windowFlagsTimer->setSingleShot(false);
        connect(m_windowFlagsTimer, &QTimer::timeout, this, [this]() {
            ensureFramelessWindow();
        });
        m_windowFlagsTimer->start();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            // 记录鼠标按下位置
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
        MainWindow::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (event->buttons() & Qt::LeftButton) {
            // 移动窗口
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
        MainWindow::mouseMoveEvent(event);
    }

    // 重写showEvent，确保窗口显示时工具栏可见
    void showEvent(QShowEvent* event) override
    {
        MainWindow::showEvent(event);
        
        // 确保窗口标志正确设置
        Qt::WindowFlags flags = windowFlags();
        if (!(flags & Qt::FramelessWindowHint)) {
            // 使用QTimer延迟设置，避免在showEvent中直接调用show()
            QTimer::singleShot(0, this, [this]() {
                setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
                show();
            });
        }
        
        // 确保样式应用
        QWidget* central = centralWidget();
        if (central) {
            central->setStyleSheet("background-color: #282A2B; color: #ffffff;");
        }
        
        // 确保工具栏始终可见
        ensureToolbarVisible();
        
        // 延迟应用样式，确保窗口完全显示后再设置
        QTimer::singleShot(10, this, [this]() {
            applyToolbarStyle();
            ensureToolbarVisible();
            
            // 再次确保样式应用到所有子控件
            QWidget* central = centralWidget();
            if (central) {
                central->setStyleSheet("background-color: #282A2B; color: #ffffff;");
            }
        });
    }

    // 重写resizeEvent，确保窗口大小改变时工具栏可见
    void resizeEvent(QResizeEvent* event) override
    {
        MainWindow::resizeEvent(event);
        
        // 确保窗口标志正确
        Qt::WindowFlags flags = windowFlags();
        if (!(flags & Qt::FramelessWindowHint)) {
            setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        }
        
        // 确保工具栏始终可见
        ensureToolbarVisible();
    }
    
    // 重写changeEvent，拦截窗口状态改变
    void changeEvent(QEvent* event) override
    {
        MainWindow::changeEvent(event);
        
        // 任何窗口状态改变后，都确保窗口标志正确
        QTimer::singleShot(0, this, [this]() {
            ensureFramelessWindow();
        });
    }
    
    // 重写eventFilter，拦截所有事件，确保窗口标志不被改变
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        // 先让父类处理事件
        bool result = MainWindow::eventFilter(obj, event);
        
        // 如果是窗口相关的事件，立即确保窗口标志正确（不延迟）
        if (obj == this && (
            event->type() == QEvent::WindowStateChange ||
            event->type() == QEvent::ActivationChange ||
            event->type() == QEvent::Show ||
            event->type() == QEvent::Resize ||
            event->type() == QEvent::Move ||
            event->type() == QEvent::Paint)) {
            // 立即检查并修复窗口标志，不延迟
            ensureFramelessWindow();
        }
        
        return result;
    }

private:
    // 确保窗口始终是无标题栏的辅助方法
    void ensureFramelessWindow()
    {
        Qt::WindowFlags flags = windowFlags();
        // 检查是否缺少 FramelessWindowHint，或者有不需要的标志
        if (!(flags & Qt::FramelessWindowHint) || 
            (flags & Qt::WindowTitleHint) ||
            (flags & Qt::WindowSystemMenuHint)) {
            // 强制设置为无标题栏窗口
            Qt::WindowFlags newFlags = Qt::Window | Qt::FramelessWindowHint;
            // 保留其他有用的标志
            if (flags & Qt::WindowMinimizeButtonHint) {
                newFlags |= Qt::WindowMinimizeButtonHint;
            }
            if (flags & Qt::WindowMaximizeButtonHint) {
                newFlags |= Qt::WindowMaximizeButtonHint;
            }
            setWindowFlags(newFlags);
            if (isVisible()) {
                show();
            }
        }
    }

private:
    // 确保工具栏可见的辅助方法
    void ensureToolbarVisible()
    {
        QWidget* central = centralWidget();
        if (central) {
            QWidget* toolbar = nullptr;
            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(central->layout());
            if (layout && layout->count() > 0) {
                QLayoutItem* item = layout->itemAt(0);
                if (item) {
                    toolbar = item->widget();
                }
            }
            if (toolbar && !toolbar->isVisible()) {
                toolbar->setVisible(true);
            }
        }
    }

    // 应用工具栏样式
    void applyToolbarStyle()
    {
        QWidget* central = centralWidget();
        if (central) {
            QWidget* toolbar = nullptr;
            QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(central->layout());
            if (layout && layout->count() > 0) {
                QLayoutItem* item = layout->itemAt(0);
                if (item) {
                    toolbar = item->widget();
                }
            }
            if (toolbar) {
                // 设置工具栏样式，确保按钮在深色主题下可见
                toolbar->setStyleSheet(
                    "QPushButton { "
                    "color: #ffffff; "
                    "background-color: rgba(255, 255, 255, 0.1); "
                    "border: 1px solid rgba(255, 255, 255, 0.2); "
                    "border-radius: 4px; "
                    "padding: 4px 8px; "
                    "} "
                    "QPushButton:hover { "
                    "background-color: rgba(255, 255, 255, 0.2); "
                    "} "
                    "QPushButton:pressed { "
                    "background-color: rgba(255, 255, 255, 0.15); "
                    "}"
                );
            }
        }
    }

private:
    QPoint m_dragPosition;  // 窗口拖动位置
    QTimer* m_toolbarCheckTimer;  // 工具栏可见性检查定时器
    QTimer* m_windowFlagsTimer;  // 窗口标志检查定时器
};


