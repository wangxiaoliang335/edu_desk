#include "ClassTeacherDelDialog.h"
#include "QGroupInfo.h"
#include "ClassTeacherDialog.h"
#include "FriendSelectDialog.h"
#include "MemberKickDialog.h"

// 解散群聊回调数据结构
struct DismissGroupCallbackData {
    QGroupInfo* dlg;
    QString groupId;
    QString userId;
    QString userName;
};

QGroupInfo::QGroupInfo(QWidget* parent)
	: QDialog(parent)
{
    
}

void QGroupInfo::initData(QString groupName, QString groupNumberId, QString classid)
{
    setWindowTitle("班级管理");
    // 设置无边框窗口
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    resize(300, 600);
    setStyleSheet("QDialog { background-color: #5C5C5C; color: white; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QPushButton { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QLabel { font-size:14px; color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QLineEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QTextEdit { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QGroupBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QTableWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; gridline-color: #5C5C5C; font-weight: bold; } "
        "QTableWidget::item { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QComboBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QCheckBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QRadioButton { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QScrollArea { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QListWidget { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QSpinBox { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QProgressBar { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; } "
        "QSlider { color: white; background-color: #5C5C5C; border: 1px solid #5C5C5C; font-weight: bold; }");
    
    // 关闭按钮（右上角）
    m_closeButton = new QPushButton(this);
    m_closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
    m_closeButton->setIconSize(QSize(22, 22));
    m_closeButton->setFixedSize(QSize(22, 22));
    m_closeButton->setStyleSheet("background: transparent;");
    m_closeButton->move(width() - 22, 0);
    m_closeButton->hide();
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);

    m_groupName = groupName;
    m_groupNumberId = groupNumberId;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 初始化REST API
    m_restAPI = new TIMRestAPI(this);
    // 注意：管理员账号信息在使用REST API时再设置，因为此时用户可能还未登录
    
    m_friendSelectDlg = new FriendSelectDialog(this);
    // 连接成员邀请成功信号，刷新成员列表
    if (m_friendSelectDlg) {
        connect(m_friendSelectDlg, &FriendSelectDialog::membersInvitedSuccess, this, [this](const QString& groupId) {
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(groupId);
        });
    }
    m_memberKickDlg = new MemberKickDialog(this);
    // 连接成员踢出成功信号，刷新成员列表
    if (m_memberKickDlg) {
        connect(m_memberKickDlg, &MemberKickDialog::membersKickedSuccess, this, [this](const QString& groupId) {
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(groupId);
        });
    }
    //m_classTeacherDelDlg = new ClassTeacherDelDialog(this);
    m_courseDlg = new CourseDialog();
    m_courseDlg->setWindowTitle("课程表");
    m_courseDlg->resize(800, 600);
    // 设置群组ID和班级ID
    m_courseDlg->setGroupId(groupNumberId);
    m_courseDlg->setClassId(classid);

    // 设置一些课程
    //m_courseDlg->setCourse(1, 0, "数学");
    //m_courseDlg->setCourse(1, 1, "音乐");
    //m_courseDlg->setCourse(2, 1, "语文", true); // 高亮
    //m_courseDlg->setCourse(3, 4, "体育", true);

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
    m_btnDismiss = new QPushButton("解散群聊");
    m_btnExit = new QPushButton("退出群聊");
    bottomBtns->addWidget(m_btnDismiss);
    bottomBtns->addWidget(m_btnExit);
    mainLayout->addLayout(bottomBtns);
    
    // 初始状态：默认都禁用，等InitGroupMember调用后再更新
    m_btnDismiss->setEnabled(false);
    m_btnExit->setEnabled(false);
    
    // 连接退出群聊按钮的点击事件
    connect(m_btnExit, &QPushButton::clicked, this, &QGroupInfo::onExitGroupClicked);
    // 连接解散群聊按钮的点击事件
    connect(m_btnDismiss, &QPushButton::clicked, this, &QGroupInfo::onDismissGroupClicked);
}

void QGroupInfo::InitGroupMember(QString group_id, QVector<GroupMemberInfo> groupMemberInfo)
{
    m_groupNumberId = group_id;
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
                        circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
                    } else if (btn->text() == "-") {
                        circleMinus = btn;
                        circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
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
            circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_friendSelectDlg)
                {
                    if (m_friendSelectDlg->isHidden())
                    {
                        // 获取当前群组成员ID列表，用于排除
                        QVector<QString> memberIds;
                        for (const auto& member : m_groupMemberInfo)
                        {
                            memberIds.append(member.member_id);
                        }
                        m_friendSelectDlg->setExcludedMemberIds(memberIds);
                        m_friendSelectDlg->setGroupId(m_groupNumberId); // 设置群组ID
                        m_friendSelectDlg->setGroupName(m_groupName); // 设置群组名称
                        m_friendSelectDlg->InitData();
                        m_friendSelectDlg->show();
                    }
                    else
                    {
                        m_friendSelectDlg->hide();
                    }
                }
            });
            circlesLayout->addWidget(circlePlus);
        }
        if (!circleMinus) {
            circleMinus = new FriendButton("-", this);
            circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_memberKickDlg)
                {
                    if (m_memberKickDlg->isHidden())
                    {
                        // 设置群组ID和名称
                        m_memberKickDlg->setGroupId(m_groupNumberId);
                        m_memberKickDlg->setGroupName(m_groupName);
                        // 初始化成员列表数据（排除群主）
                        m_memberKickDlg->InitData(m_groupMemberInfo);
                        m_memberKickDlg->show();
                    }
                    else
                    {
                        m_memberKickDlg->hide();
                    }
                }
            });
            circlesLayout->addWidget(circleMinus);
        }
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }

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
        
        // 设置成员角色
        circleBtn->setMemberRole(iter.member_role);
        
        // 只有当前用户是群主时，才启用右键菜单
        circleBtn->setContextMenuEnabled(isOwner);
        
        // 接收右键菜单信号，传递成员ID
        QString memberId = iter.member_id;
        connect(circleBtn, &FriendButton::setLeaderRequested, this, [this, memberId]() {
            onSetLeaderRequested(memberId);
        });
        connect(circleBtn, &FriendButton::cancelLeaderRequested, this, [this, memberId]() {
            onCancelLeaderRequested(memberId);
        });
        
        circleBtn->setText(iter.member_name);
        circleBtn->setProperty("member_id", iter.member_id);
        
        // 在 + 按钮之前插入成员圆圈
        circlesLayout->insertWidget(insertIndex, circleBtn);
        insertIndex++; // 更新插入位置
    }
    
    // 更新按钮状态（根据当前用户是否是群主）
    updateButtonStates();
}


void QGroupInfo::InitGroupMember()
{
    if (m_groupMemberInfo.size() == 0)
    {
        return;
    }

    //m_groupNumberId = group_id;
    //m_groupMemberInfo = groupMemberInfo;

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
                        circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
                    }
                    else if (btn->text() == "-") {
                        circleMinus = btn;
                        circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
                    }
                    else {
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
            circlePlus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(circlePlus, &FriendButton::clicked, this, [this]() {
                if (m_friendSelectDlg)
                {
                    if (m_friendSelectDlg->isHidden())
                    {
                        // 获取当前群组成员ID列表，用于排除
                        QVector<QString> memberIds;
                        for (const auto& member : m_groupMemberInfo)
                        {
                            memberIds.append(member.member_id);
                        }
                        m_friendSelectDlg->setExcludedMemberIds(memberIds);
                        m_friendSelectDlg->setGroupId(m_groupNumberId); // 设置群组ID
                        m_friendSelectDlg->setGroupName(m_groupName); // 设置群组名称
                        m_friendSelectDlg->InitData();
                        m_friendSelectDlg->show();
                    }
                    else
                    {
                        m_friendSelectDlg->hide();
                    }
                }
                });
            circlesLayout->addWidget(circlePlus);
        }
        if (!circleMinus) {
            circleMinus = new FriendButton("-", this);
            circleMinus->setContextMenuEnabled(false); // 禁用右键菜单
            connect(circleMinus, &FriendButton::clicked, this, [this]() {
                if (m_memberKickDlg)
                {
                    if (m_memberKickDlg->isHidden())
                    {
                        // 设置群组ID和名称
                        m_memberKickDlg->setGroupId(m_groupNumberId);
                        m_memberKickDlg->setGroupName(m_groupName);
                        // 初始化成员列表数据（排除群主）
                        m_memberKickDlg->InitData(m_groupMemberInfo);
                        m_memberKickDlg->show();
                    }
                    else
                    {
                        m_memberKickDlg->hide();
                    }
                }
                });
            circlesLayout->addWidget(circleMinus);
        }
    }

    QString redStyle = "background-color:red; border-radius:25px; color:white; font-weight:bold;";
    QString blueStyle = "background-color:blue; border-radius:25px; color:white; font-weight:bold;";

    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }

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

        // 设置成员角色
        circleBtn->setMemberRole(iter.member_role);
        
        // 只有当前用户是群主时，才启用右键菜单
        circleBtn->setContextMenuEnabled(isOwner);
        
        // 接收右键菜单信号，传递成员ID
        QString memberId = iter.member_id;
        connect(circleBtn, &FriendButton::setLeaderRequested, this, [this, memberId]() {
            onSetLeaderRequested(memberId);
        });
        connect(circleBtn, &FriendButton::cancelLeaderRequested, this, [this, memberId]() {
            onCancelLeaderRequested(memberId);
        });

        circleBtn->setText(iter.member_name);
        circleBtn->setProperty("member_id", iter.member_id);

        // 在 + 按钮之前插入成员圆圈
        circlesLayout->insertWidget(insertIndex, circleBtn);
        insertIndex++; // 更新插入位置
    }

    // 更新按钮状态（根据当前用户是否是群主）
    updateButtonStates();
}

void QGroupInfo::updateButtonStates()
{
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 查找当前用户在成员列表中的角色
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId) {
            if (member.member_role == "群主") {
                isOwner = true;
            }
            break;
        }
    }
    
    // 根据当前用户是否是群主来设置按钮状态
    if (m_btnDismiss && m_btnExit) {
        if (isOwner) {
            // 当前用户是群主：解散群聊按钮可用，退出群聊按钮也可用（需要先转让群主）
            m_btnDismiss->setEnabled(true);
            m_btnExit->setEnabled(true);
            qDebug() << "当前用户是群主，解散群聊按钮可用，退出群聊按钮可用（需要先转让群主）";
        } else {
            // 当前用户不是群主：解散群聊按钮禁用，退出群聊按钮可用
            m_btnDismiss->setEnabled(false);
            m_btnExit->setEnabled(true);
            qDebug() << "当前用户不是群主，解散群聊按钮禁用，退出群聊按钮可用";
        }
    }
}

void QGroupInfo::onExitGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        QMessageBox::warning(this, "错误", "群组ID为空，无法退出群聊");
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    // 检查当前用户是否是群主
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == userId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }
    
    // 如果是群主，需要检查是否有其他管理员
    if (isOwner) {
        // 查找所有管理员（排除群主自己）
        QVector<GroupMemberInfo> adminList;
        for (const auto& member : m_groupMemberInfo) {
            if (member.member_id != userId && member.member_role == "管理员") {
                adminList.append(member);
            }
        }
        
        // 如果没有其他管理员，提示错误
        if (adminList.isEmpty()) {
            QMessageBox::warning(this, "无法退出", "只有一个班主任。请先设置新班主任，再退出。");
            return;
        }
        
        // 有管理员，按好友列表顺序排序（使用成员名称排序）
        std::sort(adminList.begin(), adminList.end(), [](const GroupMemberInfo& a, const GroupMemberInfo& b) {
            return a.member_name < b.member_name;
        });
        
        // 获取第一个管理员
        QString newOwnerId = adminList[0].member_id;
        QString newOwnerName = adminList[0].member_name;
        
        // 确认对话框
        int ret = QMessageBox::question(this, "确认退出", 
            QString("确定要退出群聊 \"%1\" 吗？\n\n退出前将自动将群主身份转让给 %2（第一个管理员）。").arg(m_groupName).arg(newOwnerName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        
        if (ret != QMessageBox::Yes) {
            return;
        }
        
        // 转让群主并退出
        transferOwnerAndQuit(newOwnerId, newOwnerName);
        return;
    }
    
    // 不是群主，直接退出
    // 确认对话框
    int ret = QMessageBox::question(this, "确认退出", 
        QString("确定要退出群聊 \"%1\" 吗？").arg(m_groupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    qDebug() << "开始退出群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 构造回调数据结构
    struct ExitGroupCallbackData {
        QGroupInfo* dlg;
        QString groupId;
        QString userId;
        QString userName;
    };
    
    ExitGroupCallbackData* callbackData = new ExitGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupId = m_groupNumberId;
    callbackData->userId = userId;
    callbackData->userName = userName;
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        delete callbackData;
        return;
    }
    
    // 在使用REST API前设置管理员账号信息
    // 注意：REST API需要使用应用管理员账号，使用当前登录用户的teacher_unique_id
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    }
    
    // 调用REST API退出群聊接口
    m_restAPI->quitGroup(m_groupNumberId, userId,
        [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                qDebug() << errorMsg;
                QMessageBox::critical(callbackData->dlg, "退出失败", errorMsg);
                delete callbackData;
                return;
            }
            
            qDebug() << "REST API退出群聊成功:" << callbackData->groupId;
            
            // 立即从本地成员列表中移除当前用户
            QVector<GroupMemberInfo> updatedMemberList;
            QString leftUserId = callbackData->userId; // 记录退出的用户ID
            for (const auto& member : callbackData->dlg->m_groupMemberInfo) {
                if (member.member_id != callbackData->userId) {
                    updatedMemberList.append(member);
                }
            }
            callbackData->dlg->m_groupMemberInfo = updatedMemberList;
            
            // 立即更新UI（移除退出的成员）
            callbackData->dlg->InitGroupMember(callbackData->dlg->m_groupNumberId, callbackData->dlg->m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口（传递退出的用户ID，用于后续处理）
            callbackData->dlg->sendExitGroupRequestToServer(callbackData->groupId, callbackData->userId, leftUserId);
            
            // 释放回调数据
            delete callbackData;
        });
}

void QGroupInfo::sendExitGroupRequestToServer(const QString& groupId, const QString& userId, const QString& leftUserId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/leave";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "退出群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出成员退出群聊信号，通知父窗口刷新成员列表（传递退出的用户ID）
                    emit this->memberLeftGroup(groupId, leftUserId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "退出成功", 
                        QString("已成功退出群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    QMessageBox::warning(this, "退出失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                QMessageBox::warning(this, "退出失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送退出群聊请求到服务器失败:" << reply->errorString();
            QMessageBox::warning(this, "退出失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::onDismissGroupClicked()
{
    if (m_groupNumberId.isEmpty()) {
        QMessageBox::warning(this, "错误", "群组ID为空，无法解散群聊");
        return;
    }
    
    // 确认对话框
    int ret = QMessageBox::question(this, "确认解散", 
        QString("确定要解散群聊 \"%1\" 吗？\n解散后所有成员将被移除，此操作不可恢复！").arg(m_groupName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString userId = userInfo.teacher_unique_id;
    QString userName = userInfo.strName;
    
    qDebug() << "开始解散群聊，群组ID:" << m_groupNumberId << "，用户ID:" << userId;
    
    // 构造回调数据结构
    DismissGroupCallbackData* callbackData = new DismissGroupCallbackData;
    callbackData->dlg = this;
    callbackData->groupId = m_groupNumberId;
    callbackData->userId = userId;
    callbackData->userName = userName;
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        delete callbackData;
        return;
    }
    
    // 在使用REST API前设置管理员账号信息
    // 注意：REST API需要使用应用管理员账号，使用当前登录用户的teacher_unique_id
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    }
    
    // 调用REST API解散群聊接口
    m_restAPI->destroyGroup(m_groupNumberId,
        [=](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg;
                
                // 特殊处理常见的错误码
                if (errorCode == 10004) {
                    errorMsg = QString("解散群聊失败\n错误码: %1\n错误描述: %2\n\n注意：私有群无法解散群组。\n只有公开群、聊天室和直播大群的群主可以解散群组。").arg(errorCode).arg(errorDesc);
                } else {
                    errorMsg = QString("解散群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                }
                
                qDebug() << errorMsg;
                QMessageBox::critical(callbackData->dlg, "解散失败", errorMsg);
                
                // 释放回调数据
                delete callbackData;
                return;
            }
            
            qDebug() << "REST API解散群聊成功:" << callbackData->groupId;
            
            // REST API成功，现在调用自己的服务器接口（传递回调数据以便后续释放）
            callbackData->dlg->sendDismissGroupRequestToServer(callbackData->groupId, callbackData->userId, callbackData);
        });
}

void QGroupInfo::sendDismissGroupRequestToServer(const QString& groupId, const QString& userId, void* callbackData)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = userId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器（假设解散接口为 /groups/dismiss，如果不同请修改）
    QString url = "http://47.100.126.194:5000/groups/dismiss";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "解散群聊请求已发送到服务器";
                    
                    // 服务器响应成功，发出群聊解散信号，通知父窗口刷新群列表
                    emit this->groupDismissed(groupId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "解散成功", 
                        QString("已成功解散群聊 \"%1\"！").arg(m_groupName));
                    
                    // 关闭对话框
                    this->accept();
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    QMessageBox::warning(this, "解散失败", 
                        QString("服务器返回错误: %1").arg(message));
                }
            } else {
                qDebug() << "解析服务器响应失败";
                QMessageBox::warning(this, "解散失败", "解析服务器响应失败");
            }
        } else {
            qDebug() << "发送解散群聊请求到服务器失败:" << reply->errorString();
            QMessageBox::warning(this, "解散失败", 
                QString("网络错误: %1").arg(reply->errorString()));
        }
        
        // 释放回调数据（如果提供了）
        if (callbackData) {
            delete (DismissGroupCallbackData*)callbackData;
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::refreshMemberList(const QString& groupId)
{
    // 通知父窗口（ScheduleDialog）刷新成员列表
    emit membersRefreshed(groupId);
    qDebug() << "发出成员列表刷新信号，群组ID:" << groupId;
}

void QGroupInfo::fetchGroupMemberListFromREST(const QString& groupId)
{
    if (!m_restAPI) {
        qWarning() << "REST API未初始化，无法获取群成员列表";
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        qWarning() << "管理员账号未设置，无法获取群成员列表";
        return;
    }
    
    // 使用REST API获取群成员列表
    m_restAPI->getGroupMemberList(groupId, 100, 0, 
        [this](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                qDebug() << "获取群成员列表失败，错误码:" << errorCode << "，描述:" << errorDesc;
                return;
            }
            
            // 解析成员列表
            QVector<GroupMemberInfo> memberList;
            QJsonArray memberArray = result["MemberList"].toArray();
            
            for (const QJsonValue& value : memberArray) {
                if (!value.isObject()) continue;
                
                QJsonObject memberObj = value.toObject();
                GroupMemberInfo memberInfo;
                
                // 获取成员账号
                memberInfo.member_id = memberObj["Member_Account"].toString();
                
                // 获取成员角色
                QString role = memberObj["Role"].toString();
                if (role == "Owner") {
                    memberInfo.member_role = "群主";
                } else if (role == "Admin") {
                    memberInfo.member_role = "管理员";
                } else {
                    memberInfo.member_role = "成员";
                }
                
                // 获取成员名称（如果有）
                if (memberObj.contains("NameCard") && !memberObj["NameCard"].toString().isEmpty()) {
                    memberInfo.member_name = memberObj["NameCard"].toString();
                } else {
                    // 如果没有群名片，使用账号ID
                    memberInfo.member_name = memberInfo.member_id;
                }
                
                memberList.append(memberInfo);
            }
            
            // 合并更新成员列表（保留原有成员名称等信息）
            // 从REST API获取的成员列表可能没有成员名称，所以需要合并而不是直接替换
            int existingCount = m_groupMemberInfo.size();
            int newCount = 0;
            
            for (const GroupMemberInfo& newMember : memberList) {
                // 检查 m_groupMemberInfo 中是否已存在该成员
                bool found = false;
                for (GroupMemberInfo& existingMember : m_groupMemberInfo) {
                    if (existingMember.member_id == newMember.member_id) {
                        // 成员已存在，只更新角色信息（保留原有的成员名称）
                        existingMember.member_role = newMember.member_role;
                        found = true;
                        break;
                    }
                }
                
                // 如果成员不存在，添加到列表中
                if (!found) {
                    m_groupMemberInfo.append(newMember);
                    newCount++;
                }
            }
            
            // 刷新UI（使用更新后的完整成员列表）
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            qDebug() << "成功更新群成员列表，原有" << existingCount << "个成员，新增" 
                     << newCount << "个成员，当前共" << m_groupMemberInfo.size() << "个成员";
        });
}

void QGroupInfo::onSetLeaderRequested(const QString& memberId)
{
    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }
    
    if (!isOwner) {
        QMessageBox::warning(this, "权限不足", "只有群主可以设置管理员！");
        return;
    }
    
    // 检查成员是否已经是管理员
    bool isAlreadyAdmin = false;
    QString memberName;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == memberId) {
            memberName = member.member_name;
            if (member.member_role == "管理员") {
                isAlreadyAdmin = true;
            }
            break;
        }
    }
    
    if (isAlreadyAdmin) {
        QMessageBox::information(this, "提示", "该成员已经是管理员！");
        return;
    }
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        QMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 先获取群组信息，检查群组类型
    QStringList groupIdList;
    groupIdList.append(m_groupNumberId);
    m_restAPI->getGroupInfo(groupIdList, [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
        if (errorCode != 0) {
            QString errorMsg = QString("获取群组信息失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
            qDebug() << errorMsg;
            QMessageBox::critical(this, "错误", errorMsg);
            return;
        }
        
        // 检查群组类型
        QJsonArray groupArray = result["GroupInfo"].toArray();
        if (groupArray.isEmpty()) {
            QMessageBox::critical(this, "错误", "无法获取群组信息！");
            return;
        }
        
        QJsonObject groupInfo = groupArray[0].toObject();
        QString groupType = groupInfo["Type"].toString();
        
        // Private群组不支持设置管理员
        if (groupType == "Private") {
            QMessageBox::warning(this, "不支持的操作", 
                "私有群（Private）不支持设置管理员功能。\n\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持设置管理员。");
            return;
        }
        
        // 群组类型支持设置管理员，继续执行设置操作
        m_restAPI->modifyGroupMemberInfo(m_groupNumberId, memberId, "Admin", -1, QString(),
            [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg;
                    // 特殊处理10004错误码（群组类型不支持设置管理员）
                    if (errorCode == 10004) {
                        errorMsg = QString("设置管理员失败\n错误码: %1\n错误描述: %2\n\n该群组类型不支持设置管理员功能。\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持设置管理员。").arg(errorCode).arg(errorDesc);
                    } else {
                        errorMsg = QString("设置管理员失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                    }
                    qDebug() << errorMsg;
                    QMessageBox::critical(this, "设置失败", errorMsg);
                    return;
                }
            
            qDebug() << "REST API设置管理员成功，成员ID:" << memberId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == memberId) {
                    member.member_role = "管理员";
                    break;
                }
            }
            
            // 刷新UI
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口
            sendSetAdminRoleRequestToServer(m_groupNumberId, memberId, "管理员");
            
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(m_groupNumberId);
            
            QMessageBox::information(this, "成功", QString("已将 %1 设为管理员（班主任）！").arg(memberName));
        });
    });
}

void QGroupInfo::onCancelLeaderRequested(const QString& memberId)
{
    // 检查当前用户是否是群主
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    bool isOwner = false;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == currentUserId && member.member_role == "群主") {
            isOwner = true;
            break;
        }
    }
    
    if (!isOwner) {
        QMessageBox::warning(this, "权限不足", "只有群主可以取消管理员！");
        return;
    }
    
    // 检查成员是否是管理员
    bool isAdmin = false;
    QString memberName;
    for (const auto& member : m_groupMemberInfo) {
        if (member.member_id == memberId) {
            memberName = member.member_name;
            if (member.member_role == "管理员") {
                isAdmin = true;
            }
            break;
        }
    }
    
    if (!isAdmin) {
        QMessageBox::information(this, "提示", "该成员不是管理员！");
        return;
    }
    
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        QMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 先获取群组信息，检查群组类型
    QStringList groupIdList;
    groupIdList.append(m_groupNumberId);
    m_restAPI->getGroupInfo(groupIdList, [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
        if (errorCode != 0) {
            QString errorMsg = QString("获取群组信息失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
            qDebug() << errorMsg;
            QMessageBox::critical(this, "错误", errorMsg);
            return;
        }
        
        // 检查群组类型
        QJsonArray groupArray = result["GroupInfo"].toArray();
        if (groupArray.isEmpty()) {
            QMessageBox::critical(this, "错误", "无法获取群组信息！");
            return;
        }
        
        QJsonObject groupInfo = groupArray[0].toObject();
        QString groupType = groupInfo["Type"].toString();
        
        // Private群组不支持取消管理员
        if (groupType == "Private") {
            QMessageBox::warning(this, "不支持的操作", 
                "私有群（Private）不支持管理员功能。\n\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持管理员功能。");
            return;
        }
        
        // 群组类型支持管理员功能，继续执行取消操作
        m_restAPI->modifyGroupMemberInfo(m_groupNumberId, memberId, "Member", -1, QString(),
            [this, memberId, memberName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                if (errorCode != 0) {
                    QString errorMsg;
                    // 特殊处理10004错误码（群组类型不支持设置管理员）
                    if (errorCode == 10004) {
                        errorMsg = QString("取消管理员失败\n错误码: %1\n错误描述: %2\n\n该群组类型不支持管理员功能。\n只有公开群（Public）、聊天室（ChatRoom）和直播群（AVChatRoom）支持管理员功能。").arg(errorCode).arg(errorDesc);
                    } else {
                        errorMsg = QString("取消管理员失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                    }
                    qDebug() << errorMsg;
                    QMessageBox::critical(this, "取消失败", errorMsg);
                    return;
                }
            
            qDebug() << "REST API取消管理员成功，成员ID:" << memberId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == memberId) {
                    member.member_role = "成员";
                    break;
                }
            }
            
            // 刷新UI
            InitGroupMember(m_groupNumberId, m_groupMemberInfo);
            
            // REST API成功，现在调用自己的服务器接口
            sendSetAdminRoleRequestToServer(m_groupNumberId, memberId, "成员");
            
            // 刷新成员列表（通知父窗口刷新）
            refreshMemberList(m_groupNumberId);
            
            QMessageBox::information(this, "成功", QString("已取消 %1 的管理员身份（班主任）！").arg(memberName));
        });
    });
}

void QGroupInfo::sendSetAdminRoleRequestToServer(const QString& groupId, const QString& memberId, const QString& role)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["user_id"] = memberId;
    requestData["role"] = role; // "管理员" 或 "成员"
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/set_admin_role";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "设置管理员角色服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["code"].toInt() == 200) {
                    qDebug() << "设置管理员角色请求已发送到服务器，群组ID:" << groupId << "，成员ID:" << memberId << "，角色:" << role;
                } else {
                    QString message = obj["message"].toString();
                    qDebug() << "服务器返回错误:" << message;
                    // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
                }
            } else {
                qDebug() << "解析服务器响应失败";
            }
        } else {
            qDebug() << "发送设置管理员角色请求到服务器失败:" << reply->errorString();
            // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void QGroupInfo::transferOwnerAndQuit(const QString& newOwnerId, const QString& newOwnerName)
{
    // 检查REST API是否初始化
    if (!m_restAPI) {
        QMessageBox::critical(this, "错误", "REST API未初始化！");
        return;
    }
    
    // 获取当前用户信息
    UserInfo userInfo = CommonInfo::GetData();
    QString currentUserId = userInfo.teacher_unique_id;
    
    // 设置管理员账号信息
    std::string adminUserId = GenerateTestUserSig::instance().getAdminUserId();
    if (!adminUserId.empty()) {
        std::string adminUserSig = GenerateTestUserSig::instance().genTestUserSig(adminUserId);
        m_restAPI->setAdminInfo(QString::fromStdString(adminUserId), QString::fromStdString(adminUserSig));
    } else {
        QMessageBox::critical(this, "错误", "管理员账号未设置！");
        return;
    }
    
    // 调用腾讯IM REST API转让群主
    m_restAPI->changeGroupOwner(m_groupNumberId, newOwnerId,
        [this, newOwnerId, newOwnerName, currentUserId](int errorCode, const QString& errorDesc, const QJsonObject& result) {
            if (errorCode != 0) {
                QString errorMsg = QString("转让群主失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                qDebug() << errorMsg;
                QMessageBox::critical(this, "转让失败", errorMsg);
                return;
            }
            
            qDebug() << "REST API转让群主成功，新群主ID:" << newOwnerId;
            
            // 更新本地成员列表中的角色
            for (auto& member : m_groupMemberInfo) {
                if (member.member_id == newOwnerId) {
                    member.member_role = "群主";
                } else if (member.member_id == currentUserId) {
                    member.member_role = "成员"; // 原群主变为普通成员
                }
            }
            
            // 调用自己的服务器接口，修改新管理员为群主，并让原群主退出群组
            // 服务器接口会处理这两个操作，所以这里只需要调用服务器接口
            sendTransferOwnerRequestToServer(m_groupNumberId, currentUserId, newOwnerId);
            
            // 转让成功后，原群主退出群聊（腾讯IM层面）
            m_restAPI->quitGroup(m_groupNumberId, currentUserId,
                [this, currentUserId, newOwnerName](int errorCode, const QString& errorDesc, const QJsonObject& result) {
                    if (errorCode != 0) {
                        QString errorMsg = QString("退出群聊失败\n错误码: %1\n错误描述: %2").arg(errorCode).arg(errorDesc);
                        qDebug() << errorMsg;
                        QMessageBox::critical(this, "退出失败", errorMsg);
                        return;
                    }
                    
                    qDebug() << "REST API退出群聊成功，用户ID:" << currentUserId;
                    
                    // 立即从本地成员列表中移除当前用户
                    QVector<GroupMemberInfo> updatedMemberList;
                    for (const auto& member : m_groupMemberInfo) {
                        if (member.member_id != currentUserId) {
                            updatedMemberList.append(member);
                        }
                    }
                    m_groupMemberInfo = updatedMemberList;
                    
                    // 立即更新UI（移除退出的成员）
                    InitGroupMember(m_groupNumberId, m_groupMemberInfo);
                    
                    // 刷新成员列表（通知父窗口刷新）
                    refreshMemberList(m_groupNumberId);
                    
                    // 发出成员退出群聊信号，通知父窗口刷新群列表
                    emit this->memberLeftGroup(m_groupNumberId, currentUserId);
                    
                    // 显示成功消息
                    QMessageBox::information(this, "退出成功", 
                        QString("已成功退出群聊 \"%1\"！\n群主身份已转让给 %2。").arg(m_groupName).arg(newOwnerName));
                    
                    // 关闭对话框
                    this->accept();
                });
        });
}

void QGroupInfo::sendTransferOwnerRequestToServer(const QString& groupId, const QString& oldOwnerId, const QString& newOwnerId)
{
    // 构造发送到服务器的JSON数据
    QJsonObject requestData;
    requestData["group_id"] = groupId;
    requestData["old_owner_id"] = oldOwnerId;
    requestData["new_owner_id"] = newOwnerId;
    
    // 转换为JSON字符串
    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 发送POST请求到服务器
    QString url = "http://47.100.126.194:5000/groups/transfer_owner";
    
    // 使用QNetworkAccessManager发送POST请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(QUrl(url));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->post(networkRequest, jsonData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug() << "转让群主服务器响应:" << QString::fromUtf8(response);
            
            // 解析响应
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                int code = obj["code"].toInt();
                QString message = obj["message"].toString();
                
                if (code == 200) {
                    // 解析data字段
                    if (obj.contains("data") && obj["data"].isObject()) {
                        QJsonObject dataObj = obj["data"].toObject();
                        QString groupIdFromServer = dataObj["group_id"].toString();
                        QString groupNameFromServer = dataObj["group_name"].toString();
                        QString oldOwnerIdFromServer = dataObj["old_owner_id"].toString();
                        QString newOwnerIdFromServer = dataObj["new_owner_id"].toString();
                        QString newOwnerNameFromServer = dataObj["new_owner_name"].toString();
                        
                        qDebug() << "转让群主请求已发送到服务器成功";
                        qDebug() << "群组ID:" << groupIdFromServer;
                        qDebug() << "群组名称:" << groupNameFromServer;
                        qDebug() << "原群主ID:" << oldOwnerIdFromServer;
                        qDebug() << "新群主ID:" << newOwnerIdFromServer;
                        qDebug() << "新群主名称:" << newOwnerNameFromServer;
                    } else {
                        qDebug() << "转让群主请求已发送到服务器，群组ID:" << groupId << "，原群主ID:" << oldOwnerId << "，新群主ID:" << newOwnerId;
                    }
                } else {
                    qDebug() << "服务器返回错误，code:" << code << "，message:" << message;
                    // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
                }
            } else {
                qDebug() << "解析服务器响应失败";
            }
        } else {
            qDebug() << "发送转让群主请求到服务器失败:" << reply->errorString();
            // 不显示错误消息框，因为腾讯IM已经成功，只是服务器同步失败
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

QGroupInfo::~QGroupInfo()
{}

void QGroupInfo::enterEvent(QEvent* event)
{
    QDialog::enterEvent(event);
    if (m_closeButton)
        m_closeButton->show();
}

void QGroupInfo::leaveEvent(QEvent* event)
{
    QDialog::leaveEvent(event);
    if (m_closeButton)
        m_closeButton->hide();
}

void QGroupInfo::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    if (m_closeButton)
        m_closeButton->move(this->width() - 22, 0);
}

void QGroupInfo::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void QGroupInfo::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPos);
    }
    QDialog::mouseMoveEvent(event);
}

void QGroupInfo::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QDialog::mouseReleaseEvent(event);
}

