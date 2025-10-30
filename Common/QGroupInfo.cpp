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

    // 班级课程表按钮
    QPushButton* btnSchedule = new QPushButton("班级课程表");
    btnSchedule->setStyleSheet("background-color:green; color:white; font-weight:bold;");
    mainLayout->addWidget(btnSchedule);

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

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";

    for (auto iter : m_groupMemberInfo)
    {
        if (iter.member_role == "群主")
        {
			FriendButton* circleRed = new FriendButton("", this);
			circleRed->setStyleSheet(redStyle);

			// 接收右键菜单信号
			connect(circleRed, &FriendButton::setLeaderRequested, this, []() {
				qDebug() << "设为班主任";
			});
			connect(circleRed, &FriendButton::cancelLeaderRequested, this, []() {
				qDebug() << "取消班主任";
			});
            circleRed->setText(iter.member_name);
            circleRed->setProperty("member_id", iter.member_id);
            circlesLayout->addWidget(circleRed);
        }
        else
        {
            FriendButton* circleBlue1 = new FriendButton("", this);
			connect(circleBlue1, &FriendButton::setLeaderRequested, this, []() {
				qDebug() << "设为班主任";
			});
			connect(circleBlue1, &FriendButton::cancelLeaderRequested, this, []() {
				qDebug() << "取消班主任";
			});
            circleBlue1->setText(iter.member_name);
            circleBlue1->setProperty("member_id", iter.member_id);
            circlesLayout->addWidget(circleBlue1);
        }
    }

    //FriendButton* circleRed = new FriendButton("", this);
    //circleRed->setStyleSheet(redStyle);
    //FriendButton* circleBlue1 = new FriendButton("", this);
    //FriendButton* circleBlue2 = new FriendButton("", this);
    FriendButton* circlePlus = new FriendButton("+", this);
    FriendButton* circleMinus = new FriendButton("-", this);

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

	connect(circlePlus, &FriendButton::clicked, this, [&]() {
        if (m_classTeacherDlg && m_classTeacherDlg->isHidden())
        {
            m_classTeacherDlg->show();
        }
        else
        {
            m_classTeacherDlg->hide();
        }
	});

	connect(circleMinus, &FriendButton::clicked, this, [&]() {
		if (m_classTeacherDelDlg && m_classTeacherDelDlg->isHidden())
		{
			m_classTeacherDelDlg->show();
		}
		else
		{
			m_classTeacherDelDlg->hide();
		}
	});
    

    //circlesLayout->addWidget(circleRed);
    //circlesLayout->addWidget(circleBlue1);
    //circlesLayout->addWidget(circleBlue2);
    circlesLayout->addWidget(circlePlus);
    circlesLayout->addWidget(circleMinus);
}

QGroupInfo::~QGroupInfo()
{}

