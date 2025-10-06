#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QIcon>

class UserInfoDialog : public QDialog
{
    Q_OBJECT
public:
    UserInfoDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("我的");
        resize(500, 600);
        setStyleSheet("background-color: #2a2a2a; color: white;");

        // ===================== 顶部区域 =====================
        QHBoxLayout* topLayout = new QHBoxLayout;
        QLabel* titleLabel = new QLabel("我的");
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");

        QLabel* lblNumber = new QLabel("2");
        lblNumber->setAlignment(Qt::AlignCenter);
        lblNumber->setFixedSize(30, 30);
        lblNumber->setStyleSheet("background-color: yellow; border-radius: 15px; color: red; font-weight: bold;");

        QPushButton* btnUpgrade = new QPushButton("升级为管理员");
        btnUpgrade->setStyleSheet("background-color: lightgreen; color: black; padding:6px 10px; border-radius:6px;");

        QPushButton* btnManager = new QPushButton("管理员");
        btnManager->setStyleSheet("background-color: yellow; color: black; padding:6px 10px; border-radius:6px;");

        QPushButton* btnLogout = new QPushButton("退出登录");
        btnLogout->setStyleSheet("background-color: gray; color: white; padding:6px 10px; border-radius:6px;");

        topLayout->addWidget(lblNumber);
        topLayout->addWidget(titleLabel);
        topLayout->addStretch();
        topLayout->addWidget(btnManager);
        topLayout->addWidget(btnUpgrade);
        topLayout->addWidget(btnLogout);

        // ===================== 用户基本信息 =====================
        QGridLayout* infoLayout = new QGridLayout;
        infoLayout->setHorizontalSpacing(20);
        infoLayout->setVerticalSpacing(12);

        infoLayout->addWidget(new QLabel("性别"), 0, 0);
        infoLayout->addWidget(new QLabel("女"), 0, 1);

        infoLayout->addWidget(new QLabel("地址"), 1, 0);
        infoLayout->addWidget(new QLabel("湖北省武汉市汉江区"), 1, 1);

        infoLayout->addWidget(new QLabel("学校名"), 2, 0);
        infoLayout->addWidget(new QLabel("北京市东城区第三小学"), 2, 1);

        infoLayout->addWidget(new QLabel("学段"), 3, 0);
        infoLayout->addWidget(new QLabel("小学"), 3, 1);

        infoLayout->addWidget(new QLabel("年级"), 4, 0);
        infoLayout->addWidget(new QLabel("五年级"), 4, 1);

        infoLayout->addWidget(new QLabel("任教学科"), 5, 0);
        infoLayout->addWidget(new QLabel("语文"), 5, 1);

        infoLayout->addWidget(new QLabel("任教学段"), 6, 0);
        infoLayout->addWidget(new QLabel("六班"), 6, 1);

        // ===================== 底部按钮 =====================
        QHBoxLayout* btnBottomLayout = new QHBoxLayout;
        QPushButton* btnCancel = new QPushButton("取消");
        btnCancel->setStyleSheet("background-color: gray; color: white; padding:8px 16px; border-radius:4px;");
        QPushButton* btnOk = new QPushButton("确定");
        btnOk->setStyleSheet("background-color: #1976d2; color: white; padding:8px 16px; border-radius:4px;");

        btnBottomLayout->addStretch();
        btnBottomLayout->addWidget(btnCancel);
        btnBottomLayout->addWidget(btnOk);

        // ===================== 底部工具栏 =====================
        QHBoxLayout* toolLayout = new QHBoxLayout;
        QStringList toolNames = { "电话", "消息", "相册", "文档", "设置", "用户" };
        QStringList icons = { ":/icons/phone.png", ":/icons/message.png", ":/icons/photo.png", ":/icons/doc.png", ":/icons/settings.png", ":/icons/user.png" };

        for (int i = 0; i < toolNames.size(); ++i) {
            QPushButton* btn = new QPushButton;
            btn->setIcon(QIcon(icons[i])); // 图标文件需要在资源中准备好
            btn->setIconSize(QSize(24, 24));
            btn->setFlat(true);
            btn->setStyleSheet("QPushButton {border:none; color: white;} QPushButton:hover { color: lightblue; }");
            toolLayout->addWidget(btn);
            toolLayout->addStretch();
        }

        // ===================== 主布局 =====================
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(topLayout);
        mainLayout->addSpacing(10);
        mainLayout->addLayout(infoLayout);
        mainLayout->addSpacing(20);
        mainLayout->addLayout(btnBottomLayout);
        mainLayout->addStretch();
        mainLayout->addLayout(toolLayout);

        // 连接信号
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    }
};
