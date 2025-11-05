#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QFrame>
#include <QDir>
#include <QWebSocket>
#include <QTimer>
#include <qbuttongroup.h>
#include <qmessagebox.h>
#include "ScheduleDialog.h"
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "TaQTWebSocket.h"
#include "ImSDK/includes/TIMCloud.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QDateTime>

class ClassTeacherDialog : public QDialog
{
    Q_OBJECT
public:
    ClassTeacherDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = NULL) : QDialog(parent)
    {
        setWindowTitle("班级 / 教师选择");
        resize(420, 300);
        setStyleSheet("background-color:#dde2f0; font-size:14px;");

        m_pWs = pWs;
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
                        bool bFirst = false;
                        //fLayout->addLayout(makeRowBtn("教师", QString::number(friendsArray.size()), "blue", "white"));
                        for (int i = 0; i < friendsArray.size(); i++)
                        {
                            QJsonObject friendObj = friendsArray.at(i).toObject();

                            // teacher_info 对象
                            QJsonObject teacherInfo = friendObj.value("teacher_info").toObject();
                            int id = teacherInfo.value("id").toInt();
                            QString name = teacherInfo.value("name").toString();
                            QString subject = teacherInfo.value("subject").toString();
                            QString idCard = teacherInfo.value("id_card").toString();
                            int iteacher_unique_id = teacherInfo.value("teacher_unique_id").toInt();
                            QString teacher_unique_id = QString("%1").arg(iteacher_unique_id, 6, 10, QChar('0'));

                            // user_details 对象
                            QJsonObject userDetails = friendObj.value("user_details").toObject();
                            QString phone = userDetails.value("phone").toString();
                            QString uname = userDetails.value("name").toString();
                            QString sex = userDetails.value("sex").toString();

                            /********************************************/
                            QString avatar = userDetails.value("avatar").toString();
                            QString strIdNumber = userDetails.value("id_number").toString();
                            QString avatarBase64 = userDetails.value("avatar_base64").toString();
                            QString grade = userDetails.value("grade").toString();
                            QString class_taught = userDetails.value("class_taught").toString();

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

                            if (teacherListLayout)
                            {
                                if (false == bFirst)
                                {
                                    addPersonRow(teacherListLayout, filePath, name, phone, teacher_unique_id, grade, class_taught, sexGroup, true);
                                }
                                else
                                {
                                    addPersonRow(teacherListLayout, filePath, name, phone, teacher_unique_id, grade, class_taught, sexGroup);
                                }
                                bFirst = true;
                                //fLayout->addLayout(makePairBtn(filePath, name, "green", "white"));
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

        m_scheduleDlg = new ScheduleDialog(this, pWs);
        m_scheduleDlg->InitWebSocket();
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 顶部黄色圆形数字标签
        //QLabel* lblNum = new QLabel("2");
        //lblNum->setAlignment(Qt::AlignCenter);
        //lblNum->setFixedSize(30, 30);
        //lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        //mainLayout->addWidget(lblNum, 0, Qt::AlignCenter);

        // 班级区域
        QVBoxLayout* classLayout = new QVBoxLayout;
        QLabel* lblClassTitle = new QLabel("班级");
        lblClassTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
        classLayout->addWidget(lblClassTitle);

        QWidget* classListWidget = new QWidget;
        classListLayout = new QVBoxLayout(classListWidget);
        classListLayout->setSpacing(8);
        // 初始化班级单选分组，保证互斥
        classGroup = new QButtonGroup(this);
        classGroup->setExclusive(true);
        classLayout->addWidget(classListWidget);
        mainLayout->addLayout(classLayout);

        // 教师区域
        QVBoxLayout* teacherLayout = new QVBoxLayout;
        QLabel* lblTeacherTitle = new QLabel("教师");
        lblTeacherTitle->setStyleSheet("background-color:#3b73b8; qproperty-alignment: AlignCenter; color:white; font-weight:bold; padding:6px;");
        teacherLayout->addWidget(lblTeacherTitle);

        QWidget* teacherListWidget = new QWidget;
        teacherListLayout = new QVBoxLayout(teacherListWidget);
        teacherListLayout->setSpacing(8);
        //addPersonRow(teacherListLayout, ":/icons/avatar1.png", "软件开发工程师", true); // 这里演示一个选中状态
        //addPersonRow(teacherListLayout, ":/icons/avatar2.png", "苏州-UI-已入职");
        //addPersonRow(teacherListLayout, ":/icons/avatar3.png", "平平淡淡");
        teacherLayout->addWidget(teacherListWidget);
        mainLayout->addLayout(teacherLayout);

        // 底部按钮
        QHBoxLayout* bottomLayout = new QHBoxLayout;
        btnCancel = new QPushButton("取消");
        btnOk = new QPushButton("确定");
        btnCancel->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
        btnOk->setStyleSheet("background-color:green; color:white; padding:6px; border-radius:4px;");
        bottomLayout->addStretch();
        bottomLayout->addWidget(btnCancel);
        bottomLayout->addWidget(btnOk);
        mainLayout->addLayout(bottomLayout);

        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        //connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        connect(btnOk, &QPushButton::clicked, this, [=]() {
            if (m_scheduleDlg && m_scheduleDlg->isHidden())
            {
            QAbstractButton* checked = classGroup->checkedButton();
            if (!checked) {
                qWarning() << "没有选中的教师";
                return;
            }

            QString grade = checked->property("grade").toString();
            QString class_taught = checked->property("class_taught").toString();
            m_scheduleDlg->InitData(grade + class_taught + "的班级群", "", true);
            m_scheduleDlg->show();
            }
            accept();
        });
    }

    void InitData()
    {
        UserInfo userInfo = CommonInfo::GetData();
        if (m_httpHandler)
        {
            QString url = "http://47.100.126.194:5000/friends?";
            url += "id_card=";
            url += userInfo.strIdNumber;
            m_httpHandler->get(url);
        }

        // 如已有学校ID，则拉取班级列表
        if (!userInfo.schoolId.isEmpty())
        {
            fetchClassesByPrefix(userInfo.schoolId);
        }
    }

    QVector<QString> getNoticeMsg()
    {
        return m_NoticeMsg;
    }

    void InitWebSocket()
    {
        TaQTWebSocket::regRecvDlg(this);
        if (m_pWs)
        {
            connect(m_pWs, &TaQTWebSocket::newMessage,
                this, &ClassTeacherDialog::onWebSocketMessage);
        }

        //socket = new QWebSocket();
        //connect(socket, &QWebSocket::connected, this, &ClassTeacherDialog::onConnected);
        //connect(socket, &QWebSocket::textMessageReceived, this, &ClassTeacherDialog::onMessageReceived);
        ////connect(btnOk, &QPushButton::clicked, this, &ClassTeacherDialog::sendBroadcast);
        connect(btnOk, &QPushButton::clicked, this, &ClassTeacherDialog::sendPrivateMessage);

        //UserInfo userinfo = CommonInfo::GetData();
        //// 建立连接
        //socket->open(QUrl(QString("ws://47.100.126.194:5000/ws/%1").arg(userinfo.teacher_unique_id)));

        //// 发送心跳
        //heartbeatTimer = new QTimer(this);
        //connect(heartbeatTimer, &QTimer::timeout, this, &ClassTeacherDialog::sendHeartbeat);
        //heartbeatTimer->start(5000); // 每 5 秒一次
    }

private:
    void clearLayout(QVBoxLayout* layout)
    {
        if (!layout) return;
        QLayoutItem* child;
        while ((child = layout->takeAt(0)) != nullptr)
        {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
    }

    void fetchClassesByPrefix(const QString& schoolId)
    {
        if (schoolId.isEmpty()) return;
        QString prefix = schoolId;
        if (prefix.length() != 6 || !prefix.toInt()) {
            QMessageBox::warning(this, "错误", "请输入6位数字前缀");
            return;
        }

        QJsonObject jsonObj;
        jsonObj["prefix"] = prefix;
        QJsonDocument doc(jsonObj);
        QByteArray reqData = doc.toJson(QJsonDocument::Compact);

        QNetworkAccessManager* manager = new QNetworkAccessManager(this);
        QNetworkRequest request(QUrl("http://47.100.126.194:5000/getClassesByPrefix"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply* reply = manager->post(request, reqData);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray response_data = reply->readAll();
                QJsonDocument respDoc = QJsonDocument::fromJson(response_data);
                if (respDoc.isObject()) {
                    QJsonObject root = respDoc.object();
                    QJsonObject dataObj = root.value("data").toObject();
                    int code = dataObj.value("code").toInt();
                    QString message = dataObj.value("message").toString();
                    if (code != 200) {
                        QMessageBox::warning(this, "查询失败", message);
                        reply->deleteLater();
                        return;
                    }
                    QJsonArray classes = dataObj.value("classes").toArray();
                    clearLayout(classListLayout);
                    bool first = true;
                    for (const QJsonValue& val : classes) {
                        QJsonObject cls = val.toObject();
                        QString stage = cls.value("school_stage").toString();
                        QString grade = cls.value("grade").toString();
                        QString className = cls.value("class_name").toString();
                        QString classCode = cls.value("class_code").toString();

                        // 展示名称：学段+年级+班名 或 仅班名
                        QString display = className.isEmpty() ? (grade.isEmpty() ? stage : (stage + grade)) : (stage + grade + className);
                        addPersonRow(classListLayout, ":/icons/avatar1.png", display, "", classCode, grade, className, classGroup, first);
                        if (first) first = false;
                    }
                }
            } else {
                QMessageBox::critical(this, "网络错误", reply->errorString());
            }
            reply->deleteLater();
        });
    }

    void addPersonRow(QVBoxLayout* parentLayout, const QString& iconPath, const QString& name, const QString phone, const QString teacher_unique_id,
        QString grade, QString class_taught, QButtonGroup* pBtnGroup, bool checked = false)
    {
        QHBoxLayout* rowLayout = new QHBoxLayout;
        QRadioButton* radio = new QRadioButton;
        radio->setChecked(checked);
        radio->setProperty("phone", phone); // 可以是 int / QString / QVariant
        radio->setProperty("teacher_unique_id", teacher_unique_id); // 可以是 int / QString / QVariant 班级是班级id, 教师是教师id
        radio->setProperty("grade", grade);
        radio->setProperty("class_taught", class_taught);
        radio->setProperty("name", name);

        QLabel* avatar = new QLabel;
        avatar->setFixedSize(36, 36);
        avatar->setStyleSheet("background-color: lightgray; border-radius: 18px;");
        // 如果有头像图片资源，可以这样设置：
        QPixmap pix(iconPath);
        avatar->setPixmap(pix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        QLabel* lblName = new QLabel(name);
        rowLayout->addWidget(radio);
        rowLayout->addWidget(avatar);
        rowLayout->addWidget(lblName);
        rowLayout->addStretch();

        if (pBtnGroup)
        {
            pBtnGroup->addButton(radio);
        }

        QWidget* rowWidget = new QWidget;
        rowWidget->setLayout(rowLayout);
        parentLayout->addWidget(rowWidget);
    }
    private slots:
        // ChatDialog.cpp
        void onWebSocketMessage(const QString& msg)
        {
            qDebug() << " ClassTeacherDialog msg:" << msg; // 发信号;
            // 这里可以解析 JSON 或直接追加到聊天窗口
            //addTextMessage(":/avatar_teacher.png", "对方", msg, false);
            m_NoticeMsg.push_back(msg);
            if (m_scheduleDlg)
            {
                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
                if (parseError.error != QJsonParseError::NoError) {
                    qDebug() << "JSON parse error:" << parseError.errorString();
                }
                else {
                    if (doc.isObject()) {
                        QJsonObject obj = doc.object();
                        if (obj["type"].toString() == 3)
                        {
                            obj["group_id"].toString();
                        }
                    }
                }
            }
        }

        void onConnected() {
            //logView->append("✅ 已连接到服务端");
        }

        void onMessageReceived(const QString& msg) {
            //logView->append("📩 收到消息: " + msg);
            if (0 != msg.compare("pong") && 0 == msg.contains("不在线"))
            {
                //QMessageBox::information(NULL, "提示", msg);
                m_NoticeMsg.push_back(msg);
            }
        }

        void sendBroadcast() {
            // 先拿到当前选中的按钮
            QAbstractButton* checked = sexGroup->checkedButton();
            if (!checked) {
                qWarning() << "没有选中的教师";
                return;
            }
            // 取出按钮上绑定的私有数据
            QString phone = checked->property("phone").toString();
            QString teacher_unique_id = checked->property("teacher_unique_id").toString();
            qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

            QJsonObject obj;
            obj["teacher_unique_id"] = teacher_unique_id;
            obj["phone"] = phone;
            obj["text"] = "加好友";

            // 用 QJsonDocument 序列化
            QJsonDocument doc(obj);
            // 输出美化格式（有缩进）
            QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
            qDebug() << "美化格式:" << prettyString;

            if (!prettyString.isEmpty()) {
                //socket->sendTextMessage(prettyString);
            }
        }

        void sendPrivateMessage() {
			// 先拿到当前选中的按钮
			QAbstractButton* checked = sexGroup->checkedButton();
			if (!checked) {
				qWarning() << "没有选中的教师";
				return;
			}

            QAbstractButton* classChecked = classGroup->checkedButton();
            if (!classChecked)
            {
                qWarning() << "没有选中的班级";
                return;
            }

			// 取出按钮上绑定的私有数据
			QString phone = checked->property("phone").toString();
			QString teacher_unique_id = checked->property("teacher_unique_id").toString();
            QString grade = classChecked->property("grade").toString();
            QString class_taught = classChecked->property("class_taught").toString();
            QString name = checked->property("name").toString();
			qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

            QString class_unique_id = classChecked->property("teacher_unique_id").toString();

			//QJsonObject obj;
			//obj["teacher_unique_id"] = teacher_unique_id;
			//obj["phone"] = phone;
			//obj["text"] = "加好友";
			//obj["type"] = "1";   //1 加好友，2 解除好友, 3.创建群，4.解散群

			//// 用 QJsonDocument 序列化
			//QJsonDocument doc(obj);
			//// 输出美化格式（有缩进）
			//QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
			//qDebug() << "美化格式:" << prettyString;

			//if (!prettyString.isEmpty()) {
			//	socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
			//	//inputEdit->clear();
			//}

            UserInfo userinfo = CommonInfo::GetData();
            QString groupName = grade + class_taught + "的班级群";
            
            // 使用腾讯SDK创建群组
            createGroupWithTIMSDK(groupName, teacher_unique_id, class_unique_id, userinfo);
            
            // 创建群（保留原有的WebSocket消息逻辑）
            QJsonObject createGroupMsg;
            createGroupMsg["type"] = "3";
            createGroupMsg["permission_level"] = 1;
            createGroupMsg["headImage_path"] = "/images/group.png";
            createGroupMsg["group_type"] = 1;
            createGroupMsg["nickname"] = groupName;
            createGroupMsg["owner_id"] = userinfo.teacher_unique_id;
            createGroupMsg["owner_name"] = userinfo.strName;
            createGroupMsg["school_id"] = userinfo.schoolId;
            createGroupMsg["class_id"] = class_unique_id;
            
            // 成员数组
            QJsonArray members;
            QJsonObject m1;
            m1["unique_member_id"] = teacher_unique_id;
            m1["member_name"] = name;
            m1["group_role"] = 0;
            members.append(m1);
            createGroupMsg["members"] = members;

            // 用 QJsonDocument 序列化
            QJsonDocument doc(createGroupMsg);
            // 输出美化格式（有缩进）
            QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
            qDebug() << "美化格式:" << prettyString;

            //if (!prettyString.isEmpty()) {
            //	//socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
            //    TaQTWebSocket::sendPrivateMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
            //}
        }

        //void sendPrivateMessage() {
        //    // 先拿到当前选中的按钮
        //    QAbstractButton* checked = sexGroup->checkedButton();
        //    if (!checked) {
        //        qWarning() << "没有选中的教师";
        //        return;
        //    }
        //    // 取出按钮上绑定的私有数据
        //    QString phone = checked->property("phone").toString();
        //    QString teacher_unique_id = checked->property("teacher_unique_id").toString();
        //    qDebug() << "当前选中教师 Phone:" << phone << "  唯一编号:" << teacher_unique_id;

        //    QJsonObject obj;
        //    obj["teacher_unique_id"] = teacher_unique_id;
        //    obj["phone"] = phone;
        //    obj["text"] = "加好友";
        //    obj["type"] = "1";   //1 加好友，2 解除好友, 3.创建群，4.解散群
 
        //    // 用 QJsonDocument 序列化
        //    QJsonDocument doc(obj);
        //    // 输出美化格式（有缩进）
        //    QString prettyString = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
        //    qDebug() << "美化格式:" << prettyString;

        //    if (!prettyString.isEmpty()) {
        //        socket->sendTextMessage(QString("to:%1:%2").arg(teacher_unique_id, prettyString));
        //        //inputEdit->clear();
        //    }
        //}

        void sendHeartbeat() {
            //if (socket->state() == QAbstractSocket::ConnectedState) {
            //    socket->sendTextMessage("ping");
            //}
        }

private:
    // 将TIMResult错误码转换为错误描述字符串
    static QString getTIMResultErrorString(int errorCode) {
        switch (errorCode) {
            case TIM_SUCC:
                return "成功";
            case TIM_ERR_SDKUNINIT:
                return "ImSDK未初始化";
            case TIM_ERR_NOTLOGIN:
                return "用户未登录";
            case TIM_ERR_JSON:
                return "错误的Json格式或Json Key - 请检查JSON参数格式是否正确，特别是字段名称和数据类型";
            case TIM_ERR_PARAM:
                return "参数错误 - 请检查传入的参数是否有效";
            case TIM_ERR_CONV:
                return "无效的会话";
            case TIM_ERR_GROUP:
                return "无效的群组";
            default:
                return QString("未知错误码: %1").arg(errorCode);
        }
    }
    
    // 使用腾讯SDK创建群组
    void createGroupWithTIMSDK(const QString& groupName, const QString& teacherUniqueId, 
                               const QString& classUniqueId, const UserInfo& userinfo) {
        // 构造创建群组的参数（使用Qt JSON）
        QJsonObject createGroupParam;
        
        // 必填字段：群组名称
        if (groupName.isEmpty()) {
            QMessageBox::critical(this, "错误", "群组名称不能为空！");
            return;
        }
        createGroupParam[kTIMCreateGroupParamGroupName] = groupName;
        
        // 可选：指定群组ID，如果不指定，服务器会自动生成
        // createGroupParam[kTIMCreateGroupParamGroupId] = "class_" + classUniqueId;
        
        // 群组类型（可选，默认为Public）
        createGroupParam[kTIMCreateGroupParamGroupType] = (int)kTIMGroup_Public; // 确保是整数类型
        
        // 加群选项（可选，默认为Any）
        createGroupParam[kTIMCreateGroupParamAddOption] = (int)kTIMGroupAddOpt_Any; // 确保是整数类型
        
        // 最大成员数（可选）
        createGroupParam[kTIMCreateGroupParamMaxMemberCount] = 2000;
        
        // 群组简介（可选，但如果不为空则添加）
        QString introduction = QString("班级群：%1").arg(groupName);
        if (!introduction.isEmpty()) {
            createGroupParam[kTIMCreateGroupParamIntroduction] = introduction;
        }
        
        // 群组公告（可选，但如果不为空则添加）
        QString notification = QString("欢迎加入%1").arg(groupName);
        if (!notification.isEmpty()) {
            createGroupParam[kTIMCreateGroupParamNotification] = notification;
        }
        
        // 注意：腾讯IM SDK的自定义字段需要在控制台预先配置才能使用
        // 而且注释显示 kTIMCreateGroupParamCustomInfo 是"只读(选填)"，可能在创建时不能设置
        // 暂时不添加自定义字段，班级ID和学校ID会在上传到服务器时保存
        
        // TODO: 如果需要在腾讯IM SDK中保存自定义字段，需要：
        // 1. 在腾讯云控制台配置自定义字段（classid, schoolid）
        // 2. 然后取消下面的注释
        /*
        QJsonArray customInfoArray;
        if (!classUniqueId.isEmpty()) {
            QJsonObject customClassId;
            customClassId[kTIMGroupInfoCustemStringInfoKey] = "classid";
            customClassId[kTIMGroupInfoCustemStringInfoValue] = classUniqueId;
            customInfoArray.append(customClassId);
        }
        if (!userinfo.schoolId.isEmpty()) {
            QJsonObject customSchoolId;
            customSchoolId[kTIMGroupInfoCustemStringInfoKey] = "schoolid";
            customSchoolId[kTIMGroupInfoCustemStringInfoValue] = userinfo.schoolId;
            customInfoArray.append(customSchoolId);
        }
        if (!customInfoArray.isEmpty()) {
            createGroupParam[kTIMCreateGroupParamCustomInfo] = customInfoArray;
        }
        */
        
        // 构造初始成员数组
        // 注意：创建者会自动成为群主，不需要在成员数组中指定
        // 只添加被邀请的其他成员
        QJsonArray memberArray;
        if (teacherUniqueId != userinfo.teacher_unique_id && !teacherUniqueId.isEmpty()) {
            QJsonObject invitedMemberInfo;
            invitedMemberInfo[kTIMGroupMemberInfoIdentifier] = teacherUniqueId; // 被邀请的教师ID
            // 注意：在创建群组时，初始成员的角色应该设置为 1 (Normal)
            // 0 = None, 1 = Normal, 2 = Admin, 3 = Owner
            // 创建者自动成为Owner，所以这里只能设置为Normal
            invitedMemberInfo[kTIMGroupMemberInfoMemberRole] = (int)kTIMMemberRole_Normal; // 设置为普通成员，确保是整数类型
            memberArray.append(invitedMemberInfo);
        }
        
        // 只有当成员数组不为空时才添加
        if (!memberArray.isEmpty()) {
            createGroupParam[kTIMCreateGroupParamGroupMemberArray] = memberArray;
        }
        
        // 转换为JSON字符串
        QJsonDocument doc(createGroupParam);
        
        // 验证JSON是否有效
        if (doc.isNull() || doc.isEmpty()) {
            QMessageBox::critical(this, "错误", "生成的JSON参数为空或无效！");
            return;
        }
        
        // 使用Compact格式生成JSON，确保UTF-8编码
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 验证JSON字符串是否有效
        QJsonParseError parseError;
        QJsonDocument verifyDoc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            QString errorMsg = QString("JSON格式验证失败: %1\n位置: %2").arg(parseError.errorString()).arg(parseError.offset);
            qDebug() << errorMsg;
            QMessageBox::critical(this, "JSON格式错误", errorMsg);
            return;
        }
        
        // 输出格式化的JSON用于调试
        QByteArray formattedJson = doc.toJson(QJsonDocument::Indented);
        qDebug() << "========== 创建群组参数（格式化）==========";
        qDebug() << QString::fromUtf8(formattedJson);
        qDebug() << "========== 创建群组参数（紧凑格式，UTF-8字节）==========";
        qDebug() << jsonData;
        qDebug() << "========== 创建群组参数（字符串形式）==========";
        qDebug() << QString::fromUtf8(jsonData);
        qDebug() << "========== JSON字节数组长度 ==========";
        qDebug() << jsonData.length();
        
        // 调用腾讯SDK创建群组接口
        // 创建一个结构体来保存回调需要的数据
        struct CreateGroupCallbackData {
            ClassTeacherDialog* dlg;
            QString groupName;
            QString teacherUniqueId;
            QString classUniqueId;
            UserInfo userInfo;
        };
        
        CreateGroupCallbackData* callbackData = new CreateGroupCallbackData;
        callbackData->dlg = this;
        callbackData->groupName = groupName;
        callbackData->teacherUniqueId = teacherUniqueId;
        callbackData->classUniqueId = classUniqueId;
        callbackData->userInfo = userinfo;
        
        // 直接使用QByteArray的constData()，确保是UTF-8编码的C字符串
        const char* jsonCStr = jsonData.constData();
        
        qDebug() << "========== 传递给TIMGroupCreate的JSON字符串（UTF-8）==========";
        qDebug() << jsonCStr;
        qDebug() << "========== 传递给TIMGroupCreate的JSON字符串（QString）==========";
        qDebug() << QString::fromUtf8(jsonCStr);
        
        int ret = TIMGroupCreate(jsonCStr, 
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                CreateGroupCallbackData* data = (CreateGroupCallbackData*)user_data;
                if (!data || !data->dlg) {
                    delete data;
                    return;
                }
                
                if (ERR_SUCC != code) {
                    // 创建群组失败
                    QString errorMsg = QString("创建群组失败: %1 (错误码: %2)").arg(QString::fromUtf8(desc)).arg(code);
                    qDebug() << errorMsg;
                    QMessageBox::critical(data->dlg, "创建群组失败", errorMsg);
                    delete data;
                    return;
                }
                
                // 创建群组成功，解析返回的群组ID
                if (json_params) {
                    QJsonParseError parseError;
                    QJsonDocument respDoc = QJsonDocument::fromJson(QString::fromUtf8(json_params).toUtf8(), &parseError);
                    if (parseError.error == QJsonParseError::NoError && respDoc.isObject()) {
                        QJsonObject resultObj = respDoc.object();
                        QString groupId = resultObj[kTIMCreateGroupResultGroupId].toString();
                        qDebug() << "创建群组成功，群组ID:" << groupId;
                        
                        // 上传群组信息到服务器
                        if (data->dlg && data->dlg->m_httpHandler) {
                            data->dlg->uploadGroupInfoToServer(groupId, data->groupName, data->teacherUniqueId, 
                                                               data->classUniqueId, data->userInfo);
                        }
                        
                        QMessageBox::information(data->dlg, "创建群组成功", 
                            QString("群组创建成功！\n群组ID: %1").arg(groupId));
                    }
                }
                
                delete data;
            }, callbackData);
        
        if (TIM_SUCC != ret) {
            QString errorDesc = getTIMResultErrorString(ret);
            QString errorMsg = QString("调用TIMGroupCreate接口失败\n错误码: %1\n错误描述: %2").arg(ret).arg(errorDesc);
            qDebug() << errorMsg;
            qDebug() << "请检查上述JSON参数格式是否正确";
            QMessageBox::critical(this, "创建群组失败", errorMsg);
        }
    }
    
    // 上传群组信息到服务器
    void uploadGroupInfoToServer(const QString& groupId, const QString& groupName, 
                                 const QString& teacherUniqueId, const QString& classUniqueId,
                                 const UserInfo& userinfo) {
        // 构造群组信息对象（参考FriendGroupDialog的格式）
        QJsonObject groupObj;
        
        // 群组基础信息
        groupObj["group_id"] = groupId;
        groupObj["group_name"] = groupName;
        groupObj["group_type"] = kTIMGroup_Public; // 公开群
        groupObj["face_url"] = ""; // 创建时未设置头像
        groupObj["info_seq"] = 0;
        groupObj["latest_seq"] = 0;
        groupObj["is_shutup_all"] = false;
        
        // 群组详细信息
        groupObj["detail_group_id"] = groupId;
        groupObj["detail_group_name"] = groupName;
        groupObj["detail_group_type"] = kTIMGroup_Public;
        groupObj["detail_face_url"] = "";
        groupObj["create_time"] = QDateTime::currentDateTime().toSecsSinceEpoch(); // 当前时间戳
        groupObj["detail_info_seq"] = 0;
        groupObj["introduction"] = QString("班级群：%1").arg(groupName);
        groupObj["notification"] = QString("欢迎加入%1").arg(groupName);
        groupObj["last_info_time"] = QDateTime::currentDateTime().toSecsSinceEpoch();
        groupObj["last_msg_time"] = 0;
        groupObj["next_msg_seq"] = 0;
        // 计算成员数量：群主 + 被邀请的成员
        int memberCount = 1; // 群主
        if (teacherUniqueId != userinfo.teacher_unique_id && !teacherUniqueId.isEmpty()) {
            memberCount = 2; // 群主 + 被邀请的教师
        }
        groupObj["member_num"] = memberCount;
        groupObj["max_member_num"] = 2000;
        groupObj["online_member_num"] = 0;
        groupObj["owner_identifier"] = userinfo.teacher_unique_id; // 群主ID
        groupObj["add_option"] = kTIMGroupAddOpt_Any;
        groupObj["visible"] = 2; // 默认可见
        groupObj["searchable"] = 2; // 默认可搜索
        groupObj["detail_is_shutup_all"] = false;
        // 添加自定义字段：班级ID和学校ID（这些字段会从腾讯IM SDK的自定义字段中读取）
        groupObj["classid"] = classUniqueId;
        groupObj["schoolid"] = userinfo.schoolId;
        
        // 注意：腾讯IM SDK的自定义字段存储在 group_detial_info_custom_info 中
        // 可以通过以下方式访问：
        // QJsonArray customInfo = group[kTIMGroupDetialInfoCustomInfo].toArray();
        // 遍历 customInfo 数组，查找 key 为 "class_id" 和 "school_id" 的项
        
        // 用户在该群组中的信息（当前登录用户的信息，即群主信息）
        // 注意：member_info 是当前用户（上传者）在该群组中的信息
        QJsonObject memberInfo;
        memberInfo["user_id"] = userinfo.teacher_unique_id; // 当前用户的ID（群主）
        memberInfo["readed_seq"] = 0; // 刚创建群组，已读消息序列号为0
        memberInfo["msg_flag"] = 0; // 消息接收选项，默认为0
        memberInfo["join_time"] = QDateTime::currentDateTime().toSecsSinceEpoch(); // 加入时间（创建时间）
        memberInfo["self_role"] = (int)kTIMMemberRole_Owner; // 群主角色，确保是整数类型
        memberInfo["self_msg_flag"] = 0; // 自己的消息接收选项
        memberInfo["unread_num"] = 0; // 未读消息数，刚创建时为0
        
        groupObj["member_info"] = memberInfo;
        
        // 构造groups数组（当前用户/群主）
        QJsonArray groupsArray;
        groupsArray.append(groupObj);
        
        // 构造最终的上传JSON（当前用户/群主）
        QJsonObject uploadData;
        uploadData["user_id"] = userinfo.teacher_unique_id; // 当前用户的ID（群主）
        uploadData["groups"] = groupsArray;
        
        // 转换为JSON字符串
        QJsonDocument uploadDoc(uploadData);
        QByteArray jsonData = uploadDoc.toJson(QJsonDocument::Compact);
        
        // 上传当前用户（群主）的群组信息到服务器
        QString url = "http://47.100.126.194:5000/groups/sync";
        m_httpHandler->post(url, jsonData);
        qDebug() << "上传群主群组信息到服务器，群组ID:" << groupId;
        qDebug() << "上传JSON:" << QString::fromUtf8(jsonData);
        
        // 如果有被邀请的成员，也需要为被邀请的成员创建群组信息并上传
        if (teacherUniqueId != userinfo.teacher_unique_id && !teacherUniqueId.isEmpty()) {
            // 为被邀请的成员创建群组信息对象
            QJsonObject invitedGroupObj = groupObj; // 复制群组基本信息
            
            // 创建被邀请成员的 member_info
            QJsonObject invitedMemberInfo;
            invitedMemberInfo["user_id"] = teacherUniqueId; // 被邀请成员的ID
            invitedMemberInfo["readed_seq"] = 0;
            invitedMemberInfo["msg_flag"] = 0;
            invitedMemberInfo["join_time"] = QDateTime::currentDateTime().toSecsSinceEpoch(); // 加入时间（创建时间）
            invitedMemberInfo["self_role"] = (int)kTIMMemberRole_Normal; // 普通成员角色
            invitedMemberInfo["self_msg_flag"] = 0;
            invitedMemberInfo["unread_num"] = 0;
            
            invitedGroupObj["member_info"] = invitedMemberInfo;
            
            // 构造被邀请成员的groups数组
            QJsonArray invitedGroupsArray;
            invitedGroupsArray.append(invitedGroupObj);
            
            // 构造被邀请成员的上传JSON
            QJsonObject invitedUploadData;
            invitedUploadData["user_id"] = teacherUniqueId; // 被邀请成员的ID
            invitedUploadData["groups"] = invitedGroupsArray;
            
            // 转换为JSON字符串
            QJsonDocument invitedUploadDoc(invitedUploadData);
            QByteArray invitedJsonData = invitedUploadDoc.toJson(QJsonDocument::Compact);
            
            // 上传被邀请成员的群组信息到服务器
            m_httpHandler->post(url, invitedJsonData);
            qDebug() << "上传被邀请成员群组信息到服务器，群组ID:" << groupId << "，成员ID:" << teacherUniqueId;
            qDebug() << "上传JSON:" << QString::fromUtf8(invitedJsonData);
        }
    }

private:
    ScheduleDialog* m_scheduleDlg = NULL;
    TAHttpHandler* m_httpHandler = NULL;
    QVBoxLayout* teacherListLayout = NULL;
    QVBoxLayout* classListLayout = NULL;
    QButtonGroup* sexGroup = new QButtonGroup(this);
    QButtonGroup* classGroup = NULL;
    QString m_schoolId;
    //QWebSocket* socket = NULL;
    //QTimer* heartbeatTimer;
    QPushButton* btnOk = NULL;
    QPushButton* btnCancel = NULL;
    QVector<QString> m_NoticeMsg;
    TaQTWebSocket* m_pWs = NULL;
};
