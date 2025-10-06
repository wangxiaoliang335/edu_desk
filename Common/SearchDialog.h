#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <QFrame>

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    SearchDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("查找班级/教师/群");
        resize(500, 600);

        // 整体布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 顶部数字标签
        QLabel* lblNum = new QLabel("3");
        lblNum->setAlignment(Qt::AlignCenter);
        lblNum->setFixedSize(30, 30);
        lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        mainLayout->addWidget(lblNum, 0, Qt::AlignCenter);

        // 搜索栏
        QHBoxLayout* searchLayout = new QHBoxLayout;
        QLineEdit* editSearch = new QLineEdit;
        editSearch->setPlaceholderText("输入班级编号/教师编号/群编号");
        QPushButton* btnSearch = new QPushButton("搜索");
        btnSearch->setStyleSheet("background-color: blue; color: white; padding: 6px 12px;");
        searchLayout->addWidget(editSearch);
        searchLayout->addWidget(btnSearch);
        mainLayout->addLayout(searchLayout);

        // 分类按钮行
        QHBoxLayout* filterLayout = new QHBoxLayout;
        QPushButton* btnAll = new QPushButton("全部");
        QPushButton* btnClass = new QPushButton("班级");
        QPushButton* btnTeacher = new QPushButton("教师");
        QPushButton* btnGroup = new QPushButton("群");

        QString greenStyle = "background-color: lightgreen; color: black; padding: 6px 12px; font-weight: bold;";
        btnAll->setStyleSheet(greenStyle);
        btnClass->setStyleSheet(greenStyle);
        btnTeacher->setStyleSheet(greenStyle);
        btnGroup->setStyleSheet(greenStyle);

        filterLayout->addWidget(btnAll);
        filterLayout->addWidget(btnClass);
        filterLayout->addWidget(btnTeacher);
        filterLayout->addWidget(btnGroup);
        mainLayout->addLayout(filterLayout);

        // 列表区域
        QScrollArea* scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        QWidget* listContainer = new QWidget;
        QVBoxLayout* listLayout = new QVBoxLayout(listContainer);

        // 添加示例条目
        addListItem(listLayout, ":/icons/avatar1.png", "新🌟光", "263", "兴趣", "这里是班级的介绍，选用来分享乐趣和交流。");
        addListItem(listLayout, ":/icons/avatar2.png", "上海光切削钛金加工", "1673", "兴趣小组", "上海地区切削钛金加工交流！");
        addListItem(listLayout, ":/icons/avatar3.png", "模拟炮发烧友", "483", "机电", "发烧友交流电炮模拟相关知识。");
        addListItem(listLayout, ":/icons/avatar4.png", "鸿聚OS开源技术交流群", "1933", "开源软件", "专注于开源技术分享。");

        listLayout->addStretch();
        scrollArea->setWidget(listContainer);
        mainLayout->addWidget(scrollArea);
    }

private:
    void addListItem(QVBoxLayout* parent, const QString& iconPath,
        const QString& name, const QString& memberCount,
        const QString& tag, const QString& desc)
    {
        QHBoxLayout* itemLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel;
        avatar->setFixedSize(40, 40);
        avatar->setStyleSheet("background-color: gray; border-radius: 20px;"); // 用灰色代替头像，可以换成 QPixmap 加载图片

        QVBoxLayout* infoLayout = new QVBoxLayout;
        QLabel* lblName = new QLabel(QString("%1  ⛔ %2人  %3").arg(name).arg(memberCount).arg(tag));
        lblName->setStyleSheet("font-weight: bold;");
        QLabel* lblDesc = new QLabel(desc);
        lblDesc->setStyleSheet("color: gray; font-size: 12px;");
        infoLayout->addWidget(lblName);
        infoLayout->addWidget(lblDesc);

        QPushButton* btnJoin = new QPushButton("加入");
        btnJoin->setStyleSheet("background-color: lightblue; padding: 4px 8px;");

        itemLayout->addWidget(avatar);
        itemLayout->addLayout(infoLayout);
        itemLayout->addStretch();
        itemLayout->addWidget(btnJoin);

        QFrame* frame = new QFrame;
        frame->setLayout(itemLayout);
        frame->setFrameShape(QFrame::HLine);
        parent->addWidget(frame);
    }
};
