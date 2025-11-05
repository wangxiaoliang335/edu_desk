#pragma once
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
#include <QDir>
#include <qpainterpath.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "ScheduleDialog.h"
#include "FriendNotifyDialog.h"
#include "TACAddGroupWidget.h"
#include "GroupNotifyDialog.h"
#include "TAHttpHandler.h"
#include "ImSDK/includes/TIMCloud.h"
#include "CommonInfo.h"
#include <QMap>

class RowItem : public QFrame {
    Q_OBJECT
public:
    explicit RowItem(const QString& text, QWidget* parent = nullptr) : QFrame(parent) {
        setObjectName("RowItem");
        setFixedHeight(48);
        setStyleSheet(
            "QFrame#RowItem { background:#ffffff; border-bottom:1px solid #eaeaea; }\n"
            "QLabel { color:#222; font-size:15px; }\n"
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
    FriendGroupDialog(QWidget* parent, TaQTWebSocket* pWs)
        : QDialog(parent), m_dragging(false),
        m_backgroundColor(QColor(50, 50, 50)),
        m_borderColor(Qt::white),
        m_borderWidth(2), m_radius(6),
        m_visibleCloseButton(true)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        //setModal(true);
        setAttribute(Qt::WA_TranslucentBackground);
        m_pWs = pWs;

        setWindowTitle("好友 / 群聊");
        resize(500, 600);

        //m_scheduleDlg = new ScheduleDialog(this, pWs);
        //m_scheduleDlg->InitWebSocket();

        //fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));
        //fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));
        m_httpHandler = new TAHttpHandler(this);
        if (m_httpHandler)
        {
            connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
                //成功消息就不发送了
                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj["friends"].isArray())
                    {
                        QJsonArray friendsArray = obj.value("friends").toArray();
                        fLayout->addLayout(makeRowBtn("教师", QString::number(friendsArray.size()), "blue", "white"));
                        for (int i = 0; i < friendsArray.size(); i++)
                        {
                            QJsonObject friendObj = friendsArray.at(i).toObject();

                            // teacher_info 对象
                            QJsonObject teacherInfo = friendObj.value("teacher_info").toObject();
                            int id = teacherInfo.value("id").toInt();
                            QString name = teacherInfo.value("name").toString();
                            QString subject = teacherInfo.value("subject").toString();
                            QString idCard = teacherInfo.value("id_card").toString();

                            // user_details 对象
                            QJsonObject userDetails = friendObj.value("user_details").toObject();
                            QString phone = userDetails.value("phone").toString();
                            QString uname = userDetails.value("name").toString();
                            QString sex = userDetails.value("sex").toString();

                            /********************************************/
                            QString avatar = userDetails.value("avatar").toString();
                            QString strIdNumber = userDetails.value("id_number").toString();
                            QString avatarBase64 = userDetails.value("avatar_base64").toString();

                            // 没有文件名就用手机号或ID代替
                            if (avatar.isEmpty())
                                avatar = userDetails.value("id_number").toString() + "_" + ".png";

                            // 从最后一个 "/" 之后开始截取
                            QString fileName = avatar.section('/', -1);  // "320506197910016493_.png"
                            QString saveDir = QCoreApplication::applicationDirPath() + "/avatars/" + strIdNumber; // 保存图片目录
                            QDir().mkpath(saveDir);
                            QString filePath = saveDir + "/" + fileName;

                            if (avatarBase64.isEmpty()) {
                                qWarning() << "No avatar data for" << filePath;
                                //continue;
                            }
                            //m_userInfo.strHeadImagePath = filePath;

                            // Base64 解码成图片二进制数据
                            QByteArray imageData = QByteArray::fromBase64(avatarBase64.toUtf8());

                            // 写入文件（覆盖旧的）
                            QFile file(filePath);
                            if (!file.open(QIODevice::WriteOnly)) {
                                qWarning() << "Cannot open file for writing:" << filePath;
                                //continue;
                            }
                            file.write(imageData);
                            file.close();

                            if (fLayout)
                            {
                                fLayout->addLayout(makePairBtn(filePath, name, "green", "white", "", false));
                            }
                            /********************************************/
                        }

                        QJsonObject oTmp = obj["data"].toObject();
                        QString strTmp = oTmp["message"].toString();
                        qDebug() << "status:" << oTmp["code"].toString();
                        qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                        //errLabel->setText(strTmp);
                        //user_id = oTmp["user_id"].toInt();
                    }
                    else if (obj["data"].isObject())
                    {
                        QJsonObject dataObj = obj["data"].toObject();
                        if (dataObj["groups"].isArray())
                        {
                            gAdminLayout->addLayout(makeRowBtn("管理", "1", "orange", "black"));
                            QJsonArray groupArray = dataObj["groups"].toArray();
                            for (int i = 0; i < groupArray.size(); i++)
                            {
                                QJsonObject groupObj = groupArray.at(i).toObject();
                                QString unique_group_id = groupObj.value("unique_group_id").toString();
                                QString avatar_base64 = groupObj.value("avatar_base64").toString();
                                QString headImage_path = groupObj.value("headImage_path").toString();

                                //// 假设 avatarBase64 是 QString 类型，从 HTTP 返回的 JSON 中拿到
                                //QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());
                                //QPixmap pixmap;
                                //pixmap.loadFromData(imageData);

                                // 从最后一个 "/" 之后开始截取
                                QString fileName = headImage_path.section('/', -1);  // "320506197910016493_.png"
                                QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + unique_group_id; // 保存图片目录
                                QDir().mkpath(saveDir);
                                QString filePath = saveDir + "/" + fileName;

                                if (avatar_base64.isEmpty()) {
                                    qWarning() << "No avatar data for" << filePath;
                                    //continue;
                                }
                                //m_userInfo.strHeadImagePath = filePath;

                                // Base64 解码成图片二进制数据
                                QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());

                                // 写入文件（覆盖旧的）
                                QFile file(filePath);
                                if (!file.open(QIODevice::WriteOnly)) {
                                    qWarning() << "Cannot open file for writing:" << filePath;
                                    //continue;
                                }
                                file.write(imageData);
                                file.close();

                                //lblAvatar->setPixmap(pixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                                //lblAvatar->setScaledContents(true);

                                gAdminLayout->addLayout(makePairBtn(filePath, groupObj.value("nickname").toString(), "white", "red", unique_group_id, true));
                            }
                        }
                        else if (dataObj["joingroups"].isArray())
                        {
                            gJoinLayout->addLayout(makeRowBtn("加入", "3", "orange", "black"));
                            QJsonArray groupArray = dataObj["joingroups"].toArray();
                            for (int i = 0; i < groupArray.size(); i++)
                            {
                                QJsonObject groupObj = groupArray.at(i).toObject();
                                QString unique_group_id = groupObj.value("unique_group_id").toString();
                                QString avatar_base64 = groupObj.value("avatar_base64").toString();
                                QString headImage_path = groupObj.value("headImage_path").toString();
                                //// 假设 avatarBase64 是 QString 类型，从 HTTP 返回的 JSON 中拿到
                                //QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());
                                //QPixmap pixmap;
                                //pixmap.loadFromData(imageData);

                                // 从最后一个 "/" 之后开始截取
                                QString fileName = headImage_path.section('/', -1);  // "320506197910016493_.png"
                                QString saveDir = QCoreApplication::applicationDirPath() + "/group_images/" + unique_group_id; // 保存图片目录
                                QDir().mkpath(saveDir);
                                QString filePath = saveDir + "/" + fileName;

                                if (avatar_base64.isEmpty()) {
                                    qWarning() << "No avatar data for" << filePath;
                                    //continue;
                                }
                                //m_userInfo.strHeadImagePath = filePath;

                                // Base64 解码成图片二进制数据
                                QByteArray imageData = QByteArray::fromBase64(avatar_base64.toUtf8());

                                // 写入文件（覆盖旧的）
                                QFile file(filePath);
                                if (!file.open(QIODevice::WriteOnly)) {
                                    qWarning() << "Cannot open file for writing:" << filePath;
                                    //continue;
                                }
                                file.write(imageData);
                                file.close();

                                //lblAvatar->setPixmap(pixmap.scaled(lblAvatar->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                                //lblAvatar->setScaledContents(true);

                                gJoinLayout->addLayout(makePairBtn(filePath, groupObj.value("nickname").toString(), "white", "red", unique_group_id, false));
                            }
                        }
                    }
                }
                else
                {
                    //errLabel->setText("网络错误");
                }
                });

            connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
                //if (errLabel)
                {
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
                    if (jsonDoc.isObject()) {
                        QJsonObject obj = jsonDoc.object();
                        if (obj["data"].isObject())
                        {
                            QJsonObject oTmp = obj["data"].toObject();
                            QString strTmp = oTmp["message"].toString();
                            qDebug() << "status:" << oTmp["code"].toString();
                            qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                            //errLabel->setText(strTmp);
                        }
                    }
                    /*else
                    {
                        errLabel->setText("网络错误");
                    }*/
                }
                });
        }

        // 顶部通知区 + 添加按钮
        QHBoxLayout* topLayout = new QHBoxLayout;
        QVBoxLayout* notifyLayout = new QVBoxLayout;
        RowItem* friendNotify = new RowItem("好友通知");
        RowItem* groupNotify = new RowItem("群通知");
        //friendNotify->setStyleSheet("font-size:16px;");
        //groupNotify->setStyleSheet("font-size:16px;");
        notifyLayout->addWidget(friendNotify);
        notifyLayout->addWidget(groupNotify);

        connect(friendNotify, &RowItem::clicked, this, [=] {
            qDebug("好友通知 clicked");
            QVector<QString> vstrNotice = addGroupWidget->getNoticeMsg();
            if (friendNotifyDlg)
            {
                friendNotifyDlg->InitData(vstrNotice);
            }

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
            QVector<QString> vstrNotice = addGroupWidget->getNoticeMsg();
            if (grpNotifyDlg)
            {
                grpNotifyDlg->InitData(vstrNotice);
            }
            if (grpNotifyDlg && grpNotifyDlg->isHidden())
            {
                grpNotifyDlg->show();
            }
            else if (grpNotifyDlg && !grpNotifyDlg->isHidden())
            {
                grpNotifyDlg->hide();
            }
            });

        QHBoxLayout* closeLayout = new QHBoxLayout;
        pLabel = new QLabel(NULL);
        pLabel->setFixedSize(QSize(22, 22));

        // 关闭按钮（右上角）
        closeButton = new QPushButton(this);
        closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
        closeButton->setIconSize(QSize(22, 22));
        closeButton->setFixedSize(QSize(22, 22));
        closeButton->setStyleSheet("background: transparent;");
        closeButton->move(width() - 24, 4);
        closeButton->hide();
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

        closeLayout->addWidget(pLabel);
        closeLayout->addStretch();
        closeLayout->addWidget(closeButton);

        QPushButton* btnAdd = new QPushButton("+");
        btnAdd->setFixedSize(40, 40);
        btnAdd->setStyleSheet("background-color: blue; color: white; font-size: 20px; border-radius: 20px;");

        addGroupWidget = new TACAddGroupWidget(this, pWs);
        friendNotifyDlg = new FriendNotifyDialog(this);
        grpNotifyDlg = new GroupNotifyDialog(this);
        topLayout->addLayout(notifyLayout, 13);
        topLayout->addStretch();
        topLayout->addWidget(btnAdd, 1);
        //topLayout->addStretch();
        //topLayout->addWidget(closeButton);

        connect(btnAdd, &QPushButton::clicked, this, [=] {
            if (addGroupWidget)
            {
                addGroupWidget->show();
            }
            });

        // 中间切换按钮
        QHBoxLayout* switchLayout = new QHBoxLayout;
        QPushButton* btnGroup = new QPushButton("群聊");
        QPushButton* btnFriend = new QPushButton("好友");
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
        fLayout = new QVBoxLayout(friendPage);
        fLayout->setSpacing(10);
        fLayout->addLayout(makeRowBtn("班级", "3", "blue", "white"));
        fLayout->addLayout(makePairBtn("班级头像", "班级昵称", "orange", "black", "", false));
        fLayout->addLayout(makePairBtn("班级头像", "班级昵称", "orange", "black", "", false));
        //fLayout->addLayout(makeRowBtn("教师", "3", "blue", "white"));
        //fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));
        //fLayout->addLayout(makePairBtn("老师头像", "老师昵称", "green", "white"));

        // 群聊界面
        QWidget* groupPage = new QWidget;
        gLayout = new QVBoxLayout(groupPage);
        gAdminLayout = new QVBoxLayout(groupPage);
        gJoinLayout = new QVBoxLayout(groupPage);
        gLayout->setSpacing(10);
        //gLayout->addLayout(makePairBtn("群头像", "群昵称", "white", "red"));
        //gLayout->addLayout(makePairBtn("群头像", "群昵称", "white", "red"));

        gLayout->addLayout(makeRowBtn("班级群", "3", "blue", "white"));
        gLayout->addLayout(gAdminLayout);
        gLayout->addLayout(gJoinLayout);
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
        mainLayout->addLayout(closeLayout);
        mainLayout->addLayout(topLayout);
        mainLayout->addSpacing(10);
        mainLayout->addLayout(switchLayout);
        mainLayout->addWidget(stack);
    }

    // 帮助函数：生成一行左右两个按钮布局（相同颜色）
    static QHBoxLayout* makeRowBtn(const QString& leftText, const QString& rightText, const QString& bgColor, const QString& fgColor)
    {
        QHBoxLayout* row = new QHBoxLayout;
        QPushButton* left = new QPushButton(leftText);
        QPushButton* right = new QPushButton(rightText);
        QString style = QString("background-color:%1; color:%2; padding:6px; font-size:16px;").arg(bgColor).arg(fgColor);
        left->setStyleSheet(style);
        right->setStyleSheet(style);
        row->addWidget(left);
        row->addWidget(right);
        return row;
    }

    // 帮助函数：生成一行两个不同用途的按钮（如头像+昵称）
    QHBoxLayout* makePairBtn(const QString& leftText, const QString& rightText, const QString& bgColor, const QString& fgColor, QString unique_group_id, bool iGroupOwner)
    {
        QHBoxLayout* pair = new QHBoxLayout;
        QPushButton* left = new QPushButton();
        QPushButton* right = new QPushButton(rightText);
        QString style = QString("background-color:%1; color:%2; padding:6px; font-size:16px;").arg(bgColor).arg(fgColor);
        left->setIcon(QIcon(leftText));
        left->setStyleSheet(style);
        right->setStyleSheet(style);
        pair->addWidget(left);
        left->setProperty("unique_group_id", unique_group_id);
        left->setProperty("iGroupOwner", iGroupOwner);
        right->setProperty("unique_group_id", unique_group_id);
        right->setProperty("iGroupOwner", iGroupOwner);
        pair->addWidget(right);
        if (!unique_group_id.isEmpty())
        {
            connect(left, &QPushButton::clicked, this, [this, rightText, unique_group_id, iGroupOwner]() {
                if (!m_scheduleDlg[unique_group_id])
                {
                    m_scheduleDlg[unique_group_id] = new ScheduleDialog(this, m_pWs);
                    m_scheduleDlg[unique_group_id]->InitWebSocket();
                    QList<Notification> curNotification;
                    for (auto iter : notifications)
                    {
                        if (iter.unique_group_id == unique_group_id)
                        {
                            curNotification.append(iter);
                        }
                    }
                    m_scheduleDlg[unique_group_id]->setNoticeMsg(curNotification);
                }

                if (m_scheduleDlg[unique_group_id] && m_scheduleDlg[unique_group_id]->isHidden())
                {
                    m_scheduleDlg[unique_group_id]->InitData(rightText, unique_group_id, iGroupOwner);
                    m_scheduleDlg[unique_group_id]->show();
                }
                else
                {
					//m_scheduleDlg[unique_group_id] = new ScheduleDialog(this, m_pWs);
					//m_scheduleDlg[unique_group_id]->InitWebSocket();
                    m_scheduleDlg[unique_group_id]->hide();
                }
            });
        }
        return pair;
    }

    void InitData()
    {
        if (m_httpHandler)
        {
            UserInfo userInfo = CommonInfo::GetData();
            QString url = "http://47.100.126.194:5000/friends?";
            url += "id_card=";
            url += userInfo.strIdNumber;
            m_httpHandler->get(url);

            url = "http://47.100.126.194:5000/groups?";
            url += "group_admin_id=";
            url += userInfo.teacher_unique_id;
            m_httpHandler->get(url);

            url = "http://47.100.126.194:5000/member/groups?";
            url += "unique_member_id=";
            url += userInfo.teacher_unique_id;
            m_httpHandler->get(url);
        }
        if (addGroupWidget)
        {
            addGroupWidget->InitData();
        }

        GetGroupJoinedList();
    }

    void GetGroupJoinedList() { // 已加入群列表
    // 获取群列表
        int ret = TIMGroupGetJoinedGroupList([](int32_t code, const char* desc, const char* json_param, const void* user_data) {
            FriendGroupDialog* ths = (FriendGroupDialog*)user_data;
            
            if (strlen(json_param) == 0) {
                return;
            }
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(QByteArray(json_param), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON Parse Error:" << parseError.errorString();
                return;
            }
            if (!jsonDoc.isArray()) {
                qDebug() << "JSON is not an array!";
                return;
            }
            QJsonArray json_group_list = jsonDoc.array();
            
            // 获取当前用户信息
            UserInfo userInfo = CommonInfo::GetData();
            QString userId = userInfo.teacher_unique_id; // 或使用其他用户ID字段
            
            // 构造上传到服务器的JSON数据
            QJsonArray groupsArray;
            
            for (int i = 0; i < json_group_list.size(); i++) {
                QJsonObject group = json_group_list[i].toObject();
                
                // 获取群组基础信息
                QString groupid = group[kTIMGroupBaseInfoGroupId].toString();
                QString groupName = group[kTIMGroupBaseInfoGroupName].toString();
                
                // 构造群组信息对象
                QJsonObject groupObj;
                
                // 群组基础信息
                groupObj["group_id"] = groupid;
                groupObj["group_name"] = groupName.isEmpty() ? group[kTIMGroupDetialInfoGroupName].toString() : groupName;
                groupObj["group_type"] = group[kTIMGroupBaseInfoGroupType].toInt();
                groupObj["face_url"] = group[kTIMGroupBaseInfoFaceUrl].toString();
                groupObj["info_seq"] = group[kTIMGroupBaseInfoInfoSeq].toInt();
                groupObj["latest_seq"] = group[kTIMGroupBaseInfoLastestSeq].toInt();
                groupObj["is_shutup_all"] = group[kTIMGroupBaseInfoIsShutupAll].toBool();
                
                // 群组详细信息
                groupObj["detail_group_id"] = group[kTIMGroupDetialInfoGroupId].toString();
                groupObj["detail_group_name"] = group[kTIMGroupDetialInfoGroupName].toString();
                groupObj["detail_group_type"] = group[kTIMGroupDetialInfoGroupType].toInt();
                groupObj["detail_face_url"] = group[kTIMGroupDetialInfoFaceUrl].toString();
                groupObj["create_time"] = group[kTIMGroupDetialInfoCreateTime].toInt();
                groupObj["detail_info_seq"] = group[kTIMGroupDetialInfoInfoSeq].toInt();
                groupObj["introduction"] = group[kTIMGroupDetialInfoIntroduction].toString();
                groupObj["notification"] = group[kTIMGroupDetialInfoNotification].toString();
                groupObj["last_info_time"] = group[kTIMGroupDetialInfoLastInfoTime].toInt();
                groupObj["last_msg_time"] = group[kTIMGroupDetialInfoLastMsgTime].toInt();
                groupObj["next_msg_seq"] = group[kTIMGroupDetialInfoNextMsgSeq].toInt();
                groupObj["member_num"] = group[kTIMGroupDetialInfoMemberNum].toInt();
                groupObj["max_member_num"] = group[kTIMGroupDetialInfoMaxMemberNum].toInt();
                groupObj["online_member_num"] = group[kTIMGroupDetialInfoOnlineMemberNum].toInt();
                //groupObj["owner_identifier"] = group[kTIMGroupDetialInfoOwnerIdentifier].toString();
                groupObj["add_option"] = group[kTIMGroupDetialInfoAddOption].toInt();
                groupObj["visible"] = group[kTIMGroupDetialInfoVisible].toInt();
                groupObj["searchable"] = group[kTIMGroupDetialInfoSearchable].toInt();
                groupObj["detail_is_shutup_all"] = group[kTIMGroupDetialInfoIsShutupAll].toBool();
                
                // 读取自定义字段（班级ID和学校ID）
                QJsonArray customInfo = group[kTIMGroupDetialInfoCustomInfo].toArray();
                for (int j = 0; j < customInfo.size(); j++) {
                    QJsonObject customItem = customInfo[j].toObject();
                    QString key = customItem[kTIMGroupInfoCustemStringInfoKey].toString();
                    QString value = customItem[kTIMGroupInfoCustemStringInfoValue].toString();
                    // 兼容旧的带下划线格式和新的无下划线格式
                    if (key == "class_id" || key == "classid") {
                        groupObj["classid"] = value;
                    } else if (key == "school_id" || key == "schoolid") {
                        groupObj["schoolid"] = value;
                    }
                }
                
                // 用户在该群组中的信息
                QJsonObject selfInfo = group[kTIMGroupBaseInfoSelfInfo].toObject();
                QJsonObject memberInfo;
                memberInfo["user_id"] = userId;
                memberInfo["readed_seq"] = group[kTIMGroupBaseInfoReadedSeq].toInt();
                memberInfo["msg_flag"] = group[kTIMGroupBaseInfoMsgFlag].toInt();
                memberInfo["join_time"] = selfInfo[kTIMGroupSelfInfoJoinTime].toInt();
                memberInfo["self_role"] = selfInfo[kTIMGroupSelfInfoRole].toInt();
                memberInfo["self_msg_flag"] = selfInfo[kTIMGroupSelfInfoMsgFlag].toInt();
                memberInfo["unread_num"] = selfInfo[kTIMGroupSelfInfoUnReadNum].toInt();
                
                groupObj["member_info"] = memberInfo;
                
                groupsArray.append(groupObj);
            }
            
            // 构造最终的上传JSON
            QJsonObject uploadData;
            uploadData["user_id"] = userId;
            uploadData["groups"] = groupsArray;
            
            // 转换为JSON字符串
            QJsonDocument uploadDoc(uploadData);
            QByteArray jsonData = uploadDoc.toJson(QJsonDocument::Compact);
            
            // 上传到服务器
            if (ths->m_httpHandler) {
                QString url = "http://47.100.126.194:5000/groups/sync";
                ths->m_httpHandler->post(url, jsonData);
                qDebug() << "上传群组信息到服务器，共" << groupsArray.size() << "个群组";
            }
            
            //CIMWnd::GetInst().Logf("GroupList", kTIMLog_Info, json_param);
            }, this);
        //Logf("Init", kTIMLog_Info, "TIMGroupGetJoinedGroupList ret %d", ret);
        qDebug() << "TIMGroupGetJoinedGroupList ret:" << ret;
    }

    void InitWebSocket()
    {
        if (addGroupWidget)
        {
            addGroupWidget->InitWebSocket();
        }

        TaQTWebSocket::regRecvDlg(this);
        if (m_pWs)
        {
            connect(m_pWs, &TaQTWebSocket::newMessage,
                this, &FriendGroupDialog::onWebSocketMessage);
        }
    }

    void setTitleName(const QString& name) {
        m_titleName = name;
        update();
    }

    void visibleCloseButton(bool val)
    {
        m_visibleCloseButton = val;
    }

    void setBackgroundColor(const QColor& color) {
        m_backgroundColor = color;
        update();
    }

    void setBorderColor(const QColor& color) {
        m_borderColor = color;
        update();
    }

    void setBorderWidth(int val) {
        m_borderWidth = val;
        update();
    }

    void setRadius(int val) {
        m_radius = val;
        update();
    }

    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRect rect(0, 0, width(), height());
        QPainterPath path;
        path.addRoundedRect(rect, m_radius, m_radius);

        p.fillPath(path, QBrush(m_backgroundColor));
        QPen pen(m_borderColor, m_borderWidth);
        p.strokePath(path, pen);

        // 标题
        p.setPen(Qt::white);
        p.drawText(10, 25, m_titleName);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
    }

    void leaveEvent(QEvent* event) override
    {
        QDialog::leaveEvent(event);
        closeButton->hide();
    }

    void enterEvent(QEvent* event) override
    {
        QDialog::enterEvent(event);
        if (m_visibleCloseButton)
            closeButton->show();
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QDialog::resizeEvent(event);
        //initShow();
        closeButton->move(this->width() - 22, 0);
    }

    TACAddGroupWidget* addGroupWidget = NULL;
    FriendNotifyDialog* friendNotifyDlg = NULL;
    GroupNotifyDialog* grpNotifyDlg = NULL;

 private slots:
	 void onWebSocketMessage(const QString& msg)
	 {
         // 解析 JSON 文本
         QJsonParseError parseError;
         QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
         if (parseError.error != QJsonParseError::NoError) {
             qWarning() << "JSON解析失败:" << parseError.errorString();
             return;
         }

         if (!doc.isObject()) {
             qWarning() << "根节点不是 JSON 对象";
             return;
         }

         QJsonObject rootObj = doc.object();

         // 提取 data 数组
         if (rootObj.contains("data") && rootObj["data"].isArray()) {
             QJsonArray dataArray = rootObj["data"].toArray();
             for (auto item : dataArray) {
                 if (!item.isObject()) continue;
                 QJsonObject obj = item.toObject();

                 Notification n;
                 n.id = obj["id"].toInt();
                 n.sender_id = obj["sender_id"].toInt();
                 n.sender_name = obj["sender_name"].isNull() ? "" : obj["sender_name"].toString();
                 n.receiver_id = obj["receiver_id"].toInt();
                 n.unique_group_id = obj["unique_group_id"].isNull() ? "" : obj["unique_group_id"].toString();
                 n.group_name = obj["group_name"].isNull() ? "" : obj["group_name"].toString();
                 n.content = obj["content"].toString();
                 n.content_text = obj["content_text"].toInt();
                 n.is_read = obj["is_read"].toInt();
                 n.is_agreed = obj["is_agreed"].toInt();
                 n.remark = obj["remark"].isNull() ? "" : obj["remark"].toString();
                 n.created_at = obj["created_at"].toString();
                 n.updated_at = obj["updated_at"].toString();

                 notifications.append(n);
             }
         }
	 }

 private:
	 bool m_dragging;
	 QPoint m_dragStartPos;
	 QString m_titleName;
	 QColor m_backgroundColor;
	 QColor m_borderColor;
	 int m_borderWidth;
	 int m_radius;
     bool m_visibleCloseButton;
     QPushButton* closeButton = NULL;
     QLabel* pLabel = NULL;
     TAHttpHandler* m_httpHandler = NULL;
     QVBoxLayout* fLayout = NULL;
     QVBoxLayout* gLayout = NULL;
     QVBoxLayout* gAdminLayout = NULL;
     QVBoxLayout* gJoinLayout = NULL;
     TaQTWebSocket* m_pWs = NULL;
     QMap<QString, ScheduleDialog*> m_scheduleDlg;
     QList<Notification> notifications;
};