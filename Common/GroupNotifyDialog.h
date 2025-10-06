#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QComboBox>

class GroupNotifyDialog : public QDialog
{
    Q_OBJECT
public:
    GroupNotifyDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("群通知");
        resize(750, 700);
        setStyleSheet("background-color: #f5f5f5; font-size: 14px;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // ===== 顶部标题栏 =====
        QHBoxLayout* titleLayout = new QHBoxLayout;
        QLabel* lblTitle = new QLabel("群通知");
        lblTitle->setStyleSheet("font-weight: bold; font-size: 20px;");
        QPushButton* btnFilter = new QPushButton("🔍");   // 可替换为图标
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
        listLayout->setSpacing(12);

        // 添加两个示例条目
        addItem(listLayout,
            "王小亮", "10:53",
            "申请加入", "大幅度反对法",
            "留言: 拿了");

        addItem(listLayout,
            "王小亮", "10:51",
            "申请加入", "rttww",
            "留言: 可以吧");

        listLayout->addStretch();
        scroll->setWidget(container);
        mainLayout->addWidget(scroll);
    }

private:
    void addItem(QVBoxLayout* parent,
        const QString& name,
        const QString& time,
        const QString& action,
        const QString& groupName,
        const QString& remark)
    {
        QFrame* itemFrame = new QFrame;
        itemFrame->setStyleSheet("background-color: white; border-radius: 8px;");
        QVBoxLayout* outerLayout = new QVBoxLayout(itemFrame);

        // 第一行：头像 + 昵称 + 时间
        QHBoxLayout* topLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel;
        avatar->setFixedSize(50, 50);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 25px;");
        QLabel* lblNameTime = new QLabel(QString("<b style='color:#0055cc'>%1</b> %2").arg(name, time));
        topLayout->addWidget(avatar);
        topLayout->addWidget(lblNameTime);
        topLayout->addStretch();
        outerLayout->addLayout(topLayout);

        // 第二行：申请加入 + 群名
        QLabel* lblAction = new QLabel(QString("%1 <a href='#' style='color:#0055cc'>%2</a>").arg(action, groupName));
        lblAction->setTextFormat(Qt::RichText);
        lblAction->setOpenExternalLinks(false);
        outerLayout->addWidget(lblAction);

        // 留言
        QLabel* lblRemark = new QLabel(remark);
        lblRemark->setStyleSheet("color: gray;");
        outerLayout->addWidget(lblRemark);

        // 操作按钮行
        QHBoxLayout* btnLayout = new QHBoxLayout;
        btnLayout->addStretch();
        QPushButton* agreeBtn = new QPushButton("同意");
        QComboBox* dropDown = new QComboBox;
        dropDown->addItem("更多");
        agreeBtn->setStyleSheet("background-color:#f5f5f5; border:1px solid lightgray; padding:4px;");
        dropDown->setStyleSheet("background-color:#f5f5f5; border:1px solid lightgray;");
        btnLayout->addWidget(agreeBtn);
        btnLayout->addWidget(dropDown);
        outerLayout->addLayout(btnLayout);

        parent->addWidget(itemFrame);
    }
};
