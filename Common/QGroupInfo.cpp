#include "QGroupInfo.h"
#include "ClassTeacherDialog.h"
#include "ClassTeacherDelDialog.h"

QGroupInfo::QGroupInfo(QWidget* parent)
	: QDialog(parent)
{
    
}

void QGroupInfo::initData(QString groupName, QString groupNumberId)
{
    setWindowTitle("班级管理");
    resize(300, 600);
    setStyleSheet("QPushButton { font-size:14px; } QLabel { font-size:14px; }");

    m_groupName = groupName;
    m_groupNumberId = groupNumberId;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    m_classTeacherDlg = new ClassTeacherDialog(this);
    m_classTeacherDelDlg = new ClassTeacherDelDialog(this);
    m_courseDlg = new CourseDialog();
    m_courseDlg->setWindowTitle("课程表");
    m_courseDlg->resize(800, 600);

    // 设置一些课程
    //m_courseDlg->setCourse(1, 0, "数学");
    //m_courseDlg->setCourse(1, 1, "音乐");
    //m_courseDlg->setCourse(2, 1, "语文", true); // 高亮
    //m_courseDlg->setCourse(3, 4, "体育", true);

    m_classTeacherDlg->setWindowTitle(" ");

    // 顶部用户信息
    QHBoxLayout* topLayout = new QHBoxLayout;
    //QLabel* lblNumber = new QLabel("①");
    //lblNumber->setAlignment(Qt::AlignCenter);
    //lblNumber->setFixedSize(30, 30);
    //lblNumber->setStyleSheet("background-color: yellow; border-radius: 15px; font-weight: bold;");
    QLabel* lblAvatar = new QLabel;
    lblAvatar->setFixedSize(50, 50);
    lblAvatar->setStyleSheet("background-color: lightgray;");
    QLabel* lblInfo = new QLabel(groupName + "\n" + groupNumberId);
    QPushButton* btnMore = new QPushButton("...");
    btnMore->setFixedSize(30, 30);
    //topLayout->addWidget(lblNumber);
    topLayout->addWidget(lblAvatar);
    topLayout->addWidget(lblInfo, 1);
    topLayout->addStretch();
    topLayout->addWidget(btnMore);
    mainLayout->addLayout(topLayout);

    // 班级编号
    QLineEdit* editClassNum = new QLineEdit("2349235");
    editClassNum->setAlignment(Qt::AlignCenter);
    editClassNum->setStyleSheet("color:red; font-size:18px; font-weight:bold;");
    mainLayout->addWidget(editClassNum);

    QHBoxLayout* pHBoxLayut = new QHBoxLayout(this);
    // 班级课程表按钮
    QPushButton* btnSchedule = new QPushButton("班级课程表");
    btnSchedule->setStyleSheet("background-color:green; color:white; font-weight:bold;");

    pHBoxLayut->addWidget(btnSchedule);
    pHBoxLayut->addStretch(2);
    mainLayout->addLayout(pHBoxLayut);

    connect(btnSchedule, &QPushButton::clicked, this, [=]() {
        qDebug() << "红框区域被点击！";
        if (m_courseDlg)
        {
            m_courseDlg->show();
        }

        // 这里可以弹出输入框、打开聊天功能等
        /*if (m_chatDlg)
        {
            m_chatDlg->show();
        }*/
    });
    

    // 好友列表
    QGroupBox* groupFriends = new QGroupBox("好友列表");
    QVBoxLayout* friendsLayout = new QVBoxLayout(groupFriends);
    circlesLayout = new QHBoxLayout;

    //QString blueStyle = "background-color:blue; border-radius:15px; color:white; font-weight:bold;";
    //QString redStyle = "background-color:red; border-radius:15px; color:white; font-weight:bold;";

    //FriendButton* circleRed = new FriendButton("", this);
    //circleRed->setStyleSheet(redStyle);
    //FriendButton* circleBlue1 = new FriendButton("", this);
    //FriendButton* circleBlue2 = new FriendButton("", this);
    //FriendButton* circlePlus = new FriendButton("+", this);
    //FriendButton* circleMinus = new FriendButton("-", this);

    //// 接收右键菜单信号
    //connect(circleRed, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleRed, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue1, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue1, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });
    //connect(circleBlue2, &FriendButton::setLeaderRequested, this, []() {
    //    qDebug() << "设为班主任";
    //    });
    //connect(circleBlue2, &FriendButton::cancelLeaderRequested, this, []() {
    //    qDebug() << "取消班主任";
    //    });

    //circlesLayout->addWidget(circleRed);
    //circlesLayout->addWidget(circleBlue1);
    //circlesLayout->addWidget(circleBlue2);
    //circlesLayout->addWidget(circlePlus);
    //circlesLayout->addWidget(circleMinus);

    friendsLayout->addLayout(circlesLayout);
    mainLayout->addWidget(groupFriends);

    // 科目输入
    QGroupBox* groupSubject = new QGroupBox("科目");
    QVBoxLayout* subjectLayout = new QVBoxLayout(groupSubject);
    QLineEdit* editSubject = new QLineEdit("语文");
    editSubject->setAlignment(Qt::AlignCenter);
    editSubject->setStyleSheet("color:red; font-size:18px; font-weight:bold;");
    subjectLayout->addWidget(editSubject);
    mainLayout->addWidget(groupSubject);

    // 开启对讲(开关)
    QHBoxLayout* talkLayout = new QHBoxLayout;
    QLabel* lblTalk = new QLabel("开启对讲");
    QCheckBox* chkTalk = new QCheckBox;
    talkLayout->addWidget(lblTalk);
    talkLayout->addStretch();
    talkLayout->addWidget(chkTalk);
    mainLayout->addLayout(talkLayout);

    // 解散群聊 / 退出群聊
    QHBoxLayout* bottomBtns = new QHBoxLayout;
    QPushButton* btnDismiss = new QPushButton("解散群聊");
    QPushButton* btnExit = new QPushButton("退出群聊");
    bottomBtns->addWidget(btnDismiss);
    bottomBtns->addWidget(btnExit);
    mainLayout->addLayout(bottomBtns);
}

void QGroupInfo::InitGroupMember(QVector<GroupMemberInfo> groupMemberInfo)
{
    m_groupMemberInfo = groupMemberInfo;

    // 清空之前的成员圆圈（保留 + 和 - 按钮）
    if (circlesLayout) {
        // 先找到 + 和 - 按钮，保存它们
        FriendButton* circlePlus = nullptr;
        FriendButton* circleMinus = nullptr;
        
        // 遍历布局，找到 + 和 - 按钮，删除其他成员圆圈
        QList<QLayoutItem*> itemsToRemove;
        for (int i = 0; i < circlesLayout->count(); i++) {
            QLayoutItem* item = circlesLayout->itemAt(i);
            if (item && item->widget()) {
                FriendButton* btn = qobject_cast<FriendButton*>(item->widget());
                if (btn) {
                    if (btn->text() == "+") {
                        circlePlus = btn;
                    } else if (btn->text() == "-") {
                        circleMinus = btn;
                    } else {
                        // 成员圆圈，标记为删除
                        itemsToRemove.append(item);
                    }
                }
            }
        }
        
        // 删除成员圆圈
        for (auto item : itemsToRemove) {
            circlesLayout->removeItem(item);
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        
        // 如果 + 和 - 按钮不存在，创建它们
        if (!circlePlus) {
            circlePlus = new FriendButton("+", this);
            connect(circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_classTeacherDlg && m_classTeacherDlg->isHidden())
                {
                    m_classTeacherDlg->show();
                }
                else
                {
                    m_classTeacherDlg->hide();
                }
            });
            circlesLayout->addWidget(circlePlus);
        }
        if (!circleMinus) {
            circleMinus = new FriendButton("-", this);
            connect(circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_classTeacherDelDlg && m_classTeacherDelDlg->isHidden())
                {
                    m_classTeacherDelDlg->show();
                }
                else
                {
                    m_classTeacherDelDlg->hide();
                }
            });
            circlesLayout->addWidget(circleMinus);
        }
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 添加成员圆圈（在 + 和 - 按钮之前插入）
    // 找到 + 按钮的位置，在它之前插入成员
    int insertIndex = circlesLayout->count() - 2; // 在倒数第二个位置（+ 按钮之前）插入
    if (insertIndex < 0) insertIndex = 0;
    
    for (auto iter : m_groupMemberInfo)
    {
        FriendButton* circleBtn = nullptr;
        if (iter.member_role == "群主")
        {
            circleBtn = new FriendButton("", this);
            circleBtn->setStyleSheet(redStyle); // 群主用红色圆圈
        }
        else
        {
            circleBtn = new FriendButton("", this);
            circleBtn->setStyleSheet(blueStyle); // 其他成员用蓝色圆圈
        }
        
        // 接收右键菜单信号
        connect(circleBtn, &FriendButton::setLeaderRequested, this, []() {
            qDebug() << "设为班主任";
        });
        connect(circleBtn, &FriendButton::cancelLeaderRequested, this, []() {
            qDebug() << "取消班主任";
        });
        
        circleBtn->setText(iter.member_name);
        circleBtn->setProperty("member_id", iter.member_id);
        
        // 在 + 按钮之前插入成员圆圈
        circlesLayout->insertWidget(insertIndex, circleBtn);
        insertIndex++; // 更新插入位置
    }
}

QGroupInfo::~QGroupInfo()
{}

