#pragma once
//#pragma execution_character_set("utf-8")
#include "TABaseDialog.h"
#include "TAFloatingWidget.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPointer>
#include <QScreen>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QResizeEvent>
#include <QEvent>
#include "UserInfoDialog.h"
#include "PwdLoginModalDialog.h"
#include <QWidgetList>

class TAUserMenuDialog : public TAFloatingWidget
{
    Q_OBJECT
public:
    explicit TAUserMenuDialog(QWidget* parent = nullptr)
        : TAFloatingWidget(parent)
        , m_visibleCloseButton(true)
    {
        setMouseTracking(true);
        closeButton = new QPushButton(this);
        closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
        closeButton->setIconSize(QSize(22, 22));
        closeButton->setFixedSize(QSize(22, 22));
        closeButton->setStyleSheet("background: transparent;");
        closeButton->hide();
        connect(closeButton, &QPushButton::clicked, this, &QWidget::hide);
    }

    void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;
    }

    void InitUI()
    {
        // 隐藏标题栏（图片里没有标题栏）
        contentLayout = new QVBoxLayout();

        this->setLayout(this->contentLayout);

        // 基类外观（半透明深色、圆角）
        setBackgroundColor(QColor(0, 0, 0, 180));
        setBorderColor(QColor(255, 255, 255, 25));
        setBorderWidth(0);
        setRadius(12);

        // 内容区域的边距和间距
        contentLayout->setContentsMargins(24, 24, 24, 24);
        contentLayout->setSpacing(16);

        // 顶部：头像 + 姓名 + 电话
        QHBoxLayout* headerLayout = new QHBoxLayout;
        headerLayout->setSpacing(12);

        m_avatarLabel = new AvatarLabel;
        m_avatarLabel->setFixedSize(56, 56);
        m_avatarLabel->setAvatar(QPixmap(m_userInfo.strHeadImagePath));
        m_avatarLabel->setStyleSheet(
            "QLabel {"
            "background-color: rgba(255,255,255,0.20);"
            "border-radius: 28px;"
            "}"
        );
        // 如果你有头像图标文件，放到资源里用下面两行替换：
        // QPixmap pm(":/icons/avatar.png");
        // avatar->setPixmap(pm.scaled(56,56, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QVBoxLayout* namePhoneLayout = new QVBoxLayout;
        m_nameLabel = new QLabel(m_userInfo.strName);
        QLabel* phoneLabel = new QLabel(m_userInfo.strPhone);
        m_nameLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: 600;");
        phoneLabel->setStyleSheet("color: #B0B0B0; font-size: 12px;");
        namePhoneLayout->setContentsMargins(0, 0, 0, 0);
        namePhoneLayout->setSpacing(2);
        namePhoneLayout->addWidget(m_nameLabel);
        namePhoneLayout->addWidget(phoneLabel);

        headerLayout->addWidget(m_avatarLabel);
        headerLayout->addLayout(namePhoneLayout);
        headerLayout->addStretch();

        contentLayout->addLayout(headerLayout);

        // 菜单按钮（左对齐，透明背景，悬停高亮）
        auto makeMenuButton = [](const QString& text) {
            QPushButton* b = new QPushButton(text);
            b->setCursor(Qt::PointingHandCursor);
            b->setFlat(true);
            b->setFixedHeight(40);
            b->setStyleSheet(
                "QPushButton {"
                "background: transparent;"
                "color: #FFFFFF;"
                "font-size: 16px;"
                "text-align: left;"
                "padding-left: 8px;"
                "}"
                "QPushButton:hover { color: #A8D8FF; }"
                "QPushButton:pressed { color: #7FC6FF; }"
            );
            return b;
        };

        QPushButton* btnProfile = makeMenuButton("个人信息");
        QPushButton* btnHealth = makeMenuButton("健康管理");
        QPushButton* btnAnalyze = makeMenuButton("教学分析");
        userMenuDlg = new UserInfoDialog(this);

        //userMenuDlg->setBackgroundColor(QColor(30, 60, 90));
        //userMenuDlg->setBorderColor(Qt::yellow);
        //userMenuDlg->setBorderWidth(4);
        //userMenuDlg->setRadius(12);

        //userMenuDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
        //userMenuDlg->setBorderColor(WIDGET_BORDER_COLOR);
        //userMenuDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
        //userMenuDlg->setRadius(30);

        userMenuDlg->InitData(m_userInfo);
        userMenuDlg->InitUI();
        connect(userMenuDlg, &UserInfoDialog::userNameUpdated, this, [=](const QString& newName) {
            m_userInfo.strName = newName;
            if (m_nameLabel) {
                m_nameLabel->setText(newName);
            }
        });
        connect(userMenuDlg, &UserInfoDialog::userAvatarUpdated, this, [=](const QString& newPath) {
            m_userInfo.strHeadImagePath = newPath;
            if (m_avatarLabel) {
                m_avatarLabel->setAvatar(QPixmap(newPath));
            }
        });
        connect(userMenuDlg, &UserInfoDialog::logoutRequested, this, [=]() {
            handleLogout();
        });

        // 菜单垂直排列
        QVBoxLayout* menuLayout = new QVBoxLayout;
        menuLayout->setContentsMargins(0, 12, 0, 0);
        menuLayout->setSpacing(8);
        menuLayout->addWidget(btnProfile);
        menuLayout->addWidget(btnHealth);
        menuLayout->addWidget(btnAnalyze);
        menuLayout->addStretch();

        contentLayout->addLayout(menuLayout);

        // 示例：连接信号（你可替换成自己的槽函数）
        connect(btnProfile, &QPushButton::clicked, this, [=]() {
            qDebug("打开个人信息");
            if (userMenuDlg)
            {
                if (userMenuDlg->isHidden())
                {
                    userMenuDlg->show();
                }
                else
                {
                    userMenuDlg->hide();
                }
            }
        });
        connect(btnHealth, &QPushButton::clicked, this, [] { qDebug("打开健康管理"); });
        connect(btnAnalyze, &QPushButton::clicked, this, [] { qDebug("打开教学分析"); });
    }

    void initShow()
    {
        QRect rect = this->getScreenGeometryWithTaskbar();
        if (rect.isEmpty())
            return;
        QSize windowSize = this->size();
        int x = rect.x() + (rect.width() - windowSize.width()) *5 / 6;
        //int x = rect.x() + rect.width() - windowSize.width();
        int y = rect.y() + rect.height() - windowSize.height() - 140;
        this->move(x, y);
    }

    void enterEvent(QEvent* event) override
    {
        TAFloatingWidget::enterEvent(event);
        if (m_visibleCloseButton && closeButton) {
            closeButton->show();
        }
    }

    void leaveEvent(QEvent* event) override
    {
        TAFloatingWidget::leaveEvent(event);
        if (closeButton) {
            closeButton->hide();
        }
    }

    void resizeEvent(QResizeEvent* event) override
    {
        TAFloatingWidget::resizeEvent(event);
        if (closeButton) {
            closeButton->move(this->width() - closeButton->width() - 2, 2);
        }
    }

    UserInfoDialog* userMenuDlg = NULL;
    UserInfo m_userInfo;
    QPointer<QVBoxLayout> contentLayout;
    QLabel* m_nameLabel = nullptr;
    AvatarLabel* m_avatarLabel = nullptr;
    QPushButton* closeButton = nullptr;
    bool m_visibleCloseButton = true;
    PwdLoginModalDialog* m_loginDialog = nullptr;
    QList<QPointer<QWidget>> m_hiddenWindows;

    void showLoginDialog()
    {
        if (!m_loginDialog) {
            m_loginDialog = new PwdLoginModalDialog();
            m_loginDialog->setAttribute(Qt::WA_DeleteOnClose);
            connect(m_loginDialog, &QObject::destroyed, this, [=]() {
                m_loginDialog = nullptr;
                });
            connect(m_loginDialog, &QDialog::accepted, this, &TAUserMenuDialog::restoreWindowsAfterLogin);
        }
        m_loginDialog->InitData();
        m_loginDialog->show();
        m_loginDialog->raise();
        m_loginDialog->activateWindow();
    }

    void handleLogout()
    {
        m_hiddenWindows.clear();
        QWidgetList topWidgets = QApplication::topLevelWidgets();
        for (QWidget* widget : topWidgets) {
            if (!widget) {
                continue;
            }
            if (widget == m_loginDialog) {
                continue;
            }
            if (widget->isVisible()) {
                m_hiddenWindows.append(widget);
            }
            widget->hide();
        }
        showLoginDialog();
    }

    void restoreWindowsAfterLogin()
    {
        for (auto& widget : m_hiddenWindows) {
            if (widget) {
                widget->show();
                widget->raise();
            }
        }
        m_hiddenWindows.clear();
    }
};
