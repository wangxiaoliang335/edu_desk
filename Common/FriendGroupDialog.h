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
#include "FriendNotifyDialog.h"
#include "TACAddGroupWidget.h"
#include "GroupNotifyDialog.h"

// 帮助函数：生成一行左右两个按钮布局（相同颜色）
static QHBoxLayout* makeRowBtn(const QString& leftText, const QString& rightText,
    const QString& bgColor, const QString& fgColor)
{
    QHBoxLayout* row = new QHBoxLayout;
    QPushButton* left = new QPushButton(leftText);
    QPushButton* right = new QPushButton(rightText);
    QString style = QString("background-color:%1; color:%2; padding:6px; font-size:16px;")
        .arg(bgColor).arg(fgColor);
    left->setStyleSheet(style);
    right->setStyleSheet(style);
    row->addWidget(left);
    row->addWidget(right);
    return row;
}

// 帮助函数：生成一行两个不同用途的按钮（如头像+昵称）
static QHBoxLayout* makePairBtn(const QString& leftText, const QString& rightText,
    const QString& bgColor, const QString& fgColor)
{
    QHBoxLayout* pair = new QHBoxLayout;
    QPushButton* left = new QPushButton(leftText);
    QPushButton* right = new QPushButton(rightText);
    QString style = QString("background-color:%1; color:%2; padding:6px; font-size:16px;")
        .arg(bgColor).arg(fgColor);
    left->setStyleSheet(style);
    right->setStyleSheet(style);
    pair->addWidget(left);
    pair->addWidget(right);
    return pair;
}

class RowItem : public QFrame {
    Q_OBJECT
public:
    explicit RowItem(const QString& text, QWidget* parent = nullptr) : QFrame(parent) {
        setObjectName("RowItem");
        setFixedHeight(48);
        setStyleSheet(
            "QFrame#RowItem { background:#ffffff; border-bottom:1px solid #eaeaea; }"
            "QLabel { color:#222; font-size:15px; }"
            "QToolButton { border:none; color:#888; }"
        );
        auto h = new QHBoxLayout(this);
        h->setContentsMargins(16, 0, 16, 0);

        auto lbl = new QLabel(text);
        auto arrow = new QToolButton;
        arrow->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
        arrow->setAutoRaise(true);
        arrow->setEnabled(false); // 仅作为装饰

        h->addWidget(lbl);
        h->addStretch();
        h->addWidget(arrow);

        setCursor(Qt::PointingHandCursor);
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) emit clicked();
        QFrame::mousePressEvent(e);
    }
};

class FriendGroupDialog : public QDialog
{
    Q_OBJECT
public:
    FriendGroupDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("好友 / 群聊");
        resize(500, 600);
        setStyleSheet("background-color: #dde2f0; font-size: 16px;");

        // 顶部通知区 + 添加按钮
        QHBoxLayout* topLayout = new QHBoxLayout;
        QVBoxLayout* notifyLayout = new QVBoxLayout;
        RowItem* friendNotify = new RowItem("好友通知");
        RowItem* groupNotify = new RowItem("群通知");
        friendNotify->setStyleSheet("font-size:16px;");
        groupNotify->setStyleSheet("font-size:16px;");
        notifyLayout->addWidget(friendNotify);
        notifyLayout->addWidget(groupNotify);

        connect(friendNotify, &RowItem::clicked, this, [=] {
            qDebug("好友通知 clicked");
            if (friendNotifyDlg && friendNotifyDlg->isHidden())
            {
                friendNotifyDlg->show();
            }
            else if (friendNotifyDlg && !friendNotifyDlg->isHidden())
            {
                friendNotifyDlg->hide();
            }
        });
        connect(groupNotify, &RowItem::clicked, this, [=] {
            qDebug("群通知 clicked");
            if (grpNotifyDlg && grpNotifyDlg->isHidden())
            {
                grpNotifyDlg->show();
            }
            else if (grpNotifyDlg && !grpNotifyDlg->isHidden())
            {
                grpNotifyDlg->hide();
            }
        });

        QPushButton* btnAdd = new QPushButton("+");
        btnAdd->setFixedSize(40, 40);
        btnAdd->setStyleSheet("background-color: blue; color: white; font-size: 20px; border-radius: 20px;");

        addGroupWidget = new TACAddGroupWidget(this);
        friendNotifyDlg = new FriendNotifyDialog(this);
        grpNotifyDlg = new GroupNotifyDialog(this);
        topLayout->addLayout(notifyLayout, 13);
        topLayout->addStretch();
        topLayout->addWidget(btnAdd, 1);

		connect(btnAdd, &QPushButton::clicked, this, [=] {
			if (addGroupWidget)
			{
				addGroupWidget->show();
			}
		});

        // 中间切换按钮
        QHBoxLayout* switchLayout = new QHBoxLayout;
        QPushButton* btnFriend = new QPushButton("好友");
        QPushButton* btnGroup = new QPushButton("群聊");
        btnFriend->setCheckable(true);
        btnGroup->setCheckable(true);
        btnFriend->setStyleSheet("QPushButton {background-color: green; color: white; padding:6px;} QPushButton:checked { background-color: green;}");
        btnGroup->setStyleSheet("QPushButton {background-color: lightgray; color: white; padding:6px;} QPushButton:checked { background-color: green;}");

        switchLayout->addWidget(btnFriend);
        switchLayout->addWidget(btnGroup);

        // 内容切换区域
        QStackedWidget* stack = new QStackedWidget;

        // 好友界面
        QWidget* friendPage = new QWidget;
        QVBoxLayout* fLayout = new QVBoxLayout(friendPage);
        fLayout->setSpacing(10);
        fLayout->addLayout(makeRowBtn("班级", "3", "blue", "white"));
        fLayout->addLayout(makePairBtn("班级头像", "班级昵称", "orange", "black"));
        fLayout->addLayout(makePairBtn("班级头像", "班级昵称", "orange", "black"));
        fLayout->addLayout(makeRowBtn("教师", "3", "blue", "white"));
        fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));
        fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));

        // 群聊界面
        QWidget* groupPage = new QWidget;
        QVBoxLayout* gLayout = new QVBoxLayout(groupPage);
        gLayout->setSpacing(10);
        gLayout->addLayout(makeRowBtn("班级群", "3", "blue", "white"));
        gLayout->addLayout(makePairBtn("管理", "1", "orange", "black"));
        gLayout->addLayout(makePairBtn("加入", "3", "orange", "black"));
        gLayout->addLayout(makePairBtn("群头像", "群昵称", "white", "red"));
        gLayout->addLayout(makePairBtn("群头像", "群昵称", "white", "red"));
        gLayout->addLayout(makeRowBtn("普通群", "3", "blue", "white"));

        stack->addWidget(friendPage);
        stack->addWidget(groupPage);
        stack->setCurrentIndex(0);

        // 点击切换
        connect(btnFriend, &QPushButton::clicked, this, [=] {
            btnFriend->setChecked(true);
            btnGroup->setChecked(false);
            stack->setCurrentIndex(0);
            });
        connect(btnGroup, &QPushButton::clicked, this, [=] {
            btnFriend->setChecked(false);
            btnGroup->setChecked(true);
            stack->setCurrentIndex(1);
            });

        // 默认选中好友
        btnFriend->setChecked(true);

        // 主布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(topLayout);
        mainLayout->addSpacing(10);
        mainLayout->addLayout(switchLayout);
        mainLayout->addWidget(stack);
    }
    TACAddGroupWidget* addGroupWidget = NULL;
    FriendNotifyDialog* friendNotifyDlg = NULL;
    GroupNotifyDialog* grpNotifyDlg = NULL;
};
