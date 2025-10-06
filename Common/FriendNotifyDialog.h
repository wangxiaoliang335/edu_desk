#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>

class FriendNotifyDialog : public QDialog
{
    Q_OBJECT
public:
    FriendNotifyDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("好友通知");
        resize(750, 700);
        setStyleSheet("background-color: #f5f5f5; font-size: 14px;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // ===== 顶部标题栏 =====
        QHBoxLayout* titleLayout = new QHBoxLayout;
        QLabel* lblTitle = new QLabel("好友通知");
        lblTitle->setStyleSheet("font-weight: bold; font-size: 20px;");
        QPushButton* btnFilter = new QPushButton("🔍");   // 这里可以换成图标
        QPushButton* btnDelete = new QPushButton("🗑");
        btnFilter->setFixedSize(30, 30);
        btnDelete->setFixedSize(30, 30);
        btnFilter->setFlat(true);
        btnDelete->setFlat(true);
        titleLayout->addWidget(lblTitle);
        titleLayout->addStretch();
        titleLayout->addWidget(btnFilter);
        titleLayout->addWidget(btnDelete);
        mainLayout->addLayout(titleLayout);

        // ===== 滚动区域 =====
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        QWidget* container = new QWidget;
        QVBoxLayout* listLayout = new QVBoxLayout(container);
        listLayout->setSpacing(10);

        // 添加示例条目
        addItem(listLayout, "头像", "英语-周老师", "请求加为好友", "2025/09/09",
            "我是来自群聊“集鑫中学2025级3班”的英语-周老师", "已同意");
        addItem(listLayout, "头像", "零声教育【Mark老师】", "请求加为好友", "2025/07/13",
            "我是来自群聊“C++音视频服务器开发交流群”的零声教育", "已同意");
        addItem(listLayout, "头像", "廖深意", "请求加为好友", "2024/11/13",
            "我是来自群聊“驱动交流群”的", "已同意");
        addItem(listLayout, "头像", "a", "请求加为好友", "2024/11/13",
            "我是来自群聊“驱动交流群”的", "已同意");
        addItem(listLayout, "头像", "世纪无成", "请求加为好友", "2024/11/13",
            "我是来自群聊“WINDOWS驱动开发”的世纪无成", "已同意");
        addItem(listLayout, "头像", "Miky", "请求加为好友", "2024/11/02",
            "我是Miky Sunny，APP，小程序", "已同意");

        listLayout->addStretch();
        scroll->setWidget(container);
        mainLayout->addWidget(scroll);
    }

private:
    void addItem(QVBoxLayout* parent,
        const QString& avatarText,
        const QString& name,
        const QString& action,
        const QString& date,
        const QString& remark,
        const QString& status)
    {
        QFrame* itemFrame = new QFrame;
        itemFrame->setStyleSheet("background-color: white; border-radius: 8px;");
        QVBoxLayout* outerLayout = new QVBoxLayout(itemFrame);

        // 第一行：头像 + 名称 + 请求 + 日期
        QHBoxLayout* topLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel(avatarText);
        avatar->setFixedSize(50, 50);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 25px; text-align:center;");
        QLabel* lblName = new QLabel(QString("<b style='color:#0055cc'>%1</b> %2").arg(name, action));
        QLabel* lblDate = new QLabel(date);
        lblDate->setStyleSheet("color: gray;");
        topLayout->addWidget(avatar);
        topLayout->addWidget(lblName);
        topLayout->addStretch();
        topLayout->addWidget(lblDate);
        outerLayout->addLayout(topLayout);

        // 备注信息
        QLabel* lblRemark = new QLabel("留言: " + remark);
        lblRemark->setWordWrap(true);
        lblRemark->setStyleSheet("color: gray;");
        outerLayout->addWidget(lblRemark);

        // 状态
        QLabel* lblStatus = new QLabel(status);
        lblStatus->setStyleSheet("color: gray;");
        outerLayout->addWidget(lblStatus, 0, Qt::AlignRight);

        parent->addWidget(itemFrame);
    }
};
