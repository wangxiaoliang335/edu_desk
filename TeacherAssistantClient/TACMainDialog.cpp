#pragma execution_character_set("utf-8")
#include <QScreen>
#include "TACMainDialog.h"
#include "CommonInfo.h"

TACMainDialog::TACMainDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	this->setWindowState(Qt::WindowFullScreen);
	this->setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

TACMainDialog::~TACMainDialog()
{

}

//void TACMainDialog::InitWebSocket()
//{
//    socket = new QWebSocket();
//    connect(socket, &QWebSocket::connected, this, &TACMainDialog::onConnected);
//    connect(socket, &QWebSocket::textMessageReceived, this, &TACMainDialog::onMessageReceived);
//    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
//        this, &TACMainDialog::onError);
//
//    // 建立 WebSocket 连接
//    QString url = QString("ws://47.100.126.194:5000/ws/%1").arg(m_userInfo.strIdNumber);
//    socket->open(QUrl(url));
//
//    // 定时发送心跳
//    heartbeatTimer = new QTimer(this);
//    connect(heartbeatTimer, &QTimer::timeout, this, &TACMainDialog::sendHeartbeat);
//    heartbeatTimer->start(5000); // 每 5 秒一次
//}

void TACMainDialog::Init(QString qPhone)
{
    navBarWidget = new TACNavigationBarWidget(this);
    navBarWidget->visibleCloseButton(false);

    m_httpHandler = new TAHttpHandler(this);
    if (m_httpHandler)
    {
        connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
            //成功消息就不发送了
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8());
            if (jsonDoc.isObject()) {
                QJsonObject obj = jsonDoc.object();
                if (obj["data"].isObject())
                {
                    QJsonObject oTmp = obj["data"].toObject();
                    QString strTmp = oTmp["message"].toString();
                    qDebug() << "status:" << oTmp["code"].toString();
                    qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                    //errLabel->setText(strTmp);
                    //user_id = oTmp["user_id"].toInt();
                    if (strTmp == "登录成功")
                    {
                        accept(); // 验证通过，关闭对话框并返回 Accepted
                    }
                    else if (strTmp == "获取用户信息成功")
                    {
                        if (oTmp["userinfo"].isArray())
                        {
                            QJsonArray oUserInfo = oTmp["userinfo"].toArray();
                            if (oUserInfo.size() > 0)
                            {
                                m_userInfo.strPhone = oUserInfo.at(0)["phone"].toString();
                                m_userInfo.strName = oUserInfo.at(0)["name"].toString();
                                m_userInfo.strSex = oUserInfo.at(0)["sex"].toString();
                                m_userInfo.strAddress = oUserInfo.at(0)["address"].toString();
                                m_userInfo.strSchoolName = oUserInfo.at(0)["school_name"].toString();
                                m_userInfo.strGradeLevel = oUserInfo.at(0)["grade_level"].toString();
                                m_userInfo.strGrade = oUserInfo.at(0)["grade"].toString();
                                m_userInfo.strSubject = oUserInfo.at(0)["subject"].toString();
                                m_userInfo.strClassTaught = oUserInfo.at(0)["class_taught"].toString();
                                m_userInfo.strIsAdministrator = oUserInfo.at(0)["is_administrator"].toString();
                                m_userInfo.avatar = oUserInfo.at(0)["avatar"].toString();
                                m_userInfo.strIdNumber = oUserInfo.at(0)["id_number"].toString();
                                int iteacher_unique_id = oUserInfo.at(0)["teacher_unique_id"].toInt();
                                m_userInfo.teacher_unique_id = QString("%1").arg(iteacher_unique_id, 6, 10, QChar('0'));
                                QString avatarBase64 = oUserInfo.at(0)["avatar_base64"].toString();

                                // 没有文件名就用手机号或ID代替
                                if (m_userInfo.avatar.isEmpty())
                                    m_userInfo.avatar = oUserInfo.at(0)["id_number"].toString() + "_" + ".png";

                                // 从最后一个 "/" 之后开始截取
                                QString fileName = m_userInfo.avatar.section('/', -1);  // "320506197910016493_.png"

                                QString saveDir = QCoreApplication::applicationDirPath() + "/avatars/" + m_userInfo.strIdNumber; // 保存图片目录
                                QDir().mkpath(saveDir);
                                
                                QString filePath = saveDir + "/" + fileName;

                                if (avatarBase64.isEmpty()) {
                                    qWarning() << "No avatar data for" << filePath;
                                    //continue;
                                }

                                m_userInfo.strHeadImagePath = filePath;

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

                                if (userMenuDlg)
                                {
                                    userMenuDlg->InitData(m_userInfo);
                                    userMenuDlg->InitUI();
                                    CommonInfo::InitData(m_userInfo);
                                    if (friendGrpDlg)
                                    {
                                        friendGrpDlg->InitWebSocket();
                                    }
                                    //InitWebSocket();
                                }

                                if (schoolInfoDlg)
                                {
                                    schoolInfoDlg->InitData(m_userInfo);
                                }

                            }
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

    connect(navBarWidget, &TACNavigationBarWidget::navType, this, [=](TACNavigationBarWidgetType type,bool checked) {
        if (type == TACNavigationBarWidgetType::TIMER)
        {
            if (datetimeDialog)
            {
                datetimeDialog->show();
            }
            if (datetimeWidget)
                datetimeWidget->show();
        }
        else if (type == TACNavigationBarWidgetType::FOLDER)
        {
            if (desktopManagerWidget)
            {
                QRect rect = navBarWidget->geometry();
                desktopManagerWidget->move(rect.x(), rect.y() - desktopManagerWidget->height() - 10);
                if (checked)
                    desktopManagerWidget->show();
                else
                    desktopManagerWidget->hide();
            }
        }
        else if (type == TACNavigationBarWidgetType::USER)
        {
            if (friendGrpDlg)
                friendGrpDlg->show();
        }
        else if (type == TACNavigationBarWidgetType::WALLPAPER)
        {
            if (wallpaperLibraryDialog)
                wallpaperLibraryDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::HOMEWORK)
        {
            if (homeworkDialog)
                homeworkDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::MESSAGE)
        {
            
        }
        else if (type == TACNavigationBarWidgetType::IM)
        {
            if (imDialog)
                imDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::PREPARE_CLASS)
        {
            if (prepareClassDialog)
                prepareClassDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::CALENDAR)
        {
            if (classWeekCourseScheduldDialog)
                classWeekCourseScheduldDialog->show();
        }
        else if (type == TACNavigationBarWidgetType::USER1)
        {
            if (userMenuDlg)
            {
                userMenuDlg->show();
            }
        }
    });
    navBarWidget->show();

    prepareClassDialog = new TACPrepareClassDialog(this);
    desktopManagerWidget = new TACDesktopManagerWidget(this);
    schoolInfoDlg = new SchoolInfoDialog(this);
    trayWidget = new TACTrayWidget(this);
    connect(trayWidget, &TACTrayWidget::navType, this, [=](bool checked) {
        if (desktopManagerWidget)
        {
            QRect rect = trayWidget->geometry();
            desktopManagerWidget->move(rect.x() - desktopManagerWidget->width() - 10, rect.y() - desktopManagerWidget->height() + 180);
            if (checked)
                desktopManagerWidget->show();
            else
                desktopManagerWidget->hide();
        }
     });

    connect(trayWidget, &TACTrayWidget::navChoolInfo, this, [=](bool checked) {
        if (schoolInfoDlg)
        {
            // 获取主屏幕几何信息（Qt 5.14+ 推荐 QScreen）
            QScreen* screen = QApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            int x = (screenGeometry.width() - schoolInfoDlg->width()) / 2;
            int y = (screenGeometry.height() - schoolInfoDlg->height()) / 2;
            schoolInfoDlg->move(x, y);
            if (checked)
                schoolInfoDlg->show();
            else
                schoolInfoDlg->hide();
        }
        });

    imDialog = new TACIMDialog(this);
    friendGrpDlg = new FriendGroupDialog(this);
    friendGrpDlg->setBackgroundColor(WIDGET_BACKGROUND_COLOR);
    friendGrpDlg->setBorderColor(WIDGET_BORDER_COLOR);
    friendGrpDlg->setBorderWidth(WIDGET_BORDER_WIDTH);
    friendGrpDlg->setRadius(30);

    homeworkDialog = new TACHomeworkDialog(this);
    homeworkDialog->setContent("语文<br>背诵《荷塘月色》第二段");

    wallpaperLibraryDialog = new TACWallpaperLibraryDialog(this);
    userMenuDlg = new TAUserMenuDialog(this);

    countDownDialog = new TACCountDownDialog(this);
    countDownWidget = new TACCountDownWidget(this);
    connect(countDownWidget, &TACCountDownWidget::doubleClicked, this, [=]() {
        countDownDialog->show();
        });
    countDownWidget->setContent(countDownDialog->content());
    countDownWidget->show();

    datetimeDialog = new TACDateTimeDialog(this);
    connect(datetimeDialog, &TACDateTimeDialog::updateType, this, [=](int type) {
        datetimeWidget->setType(type);
    });

    datetimeWidget = new TACDateTimeWidget(this);
    datetimeWidget->show();

    logoWidget = new TACLogoWidget(this);
    logoWidget->updateLogo(".\\res\\img\\qinghua.png");
    logoWidget->show();

    schoolLabelWidget = new TACSchoolLabelWidget(this);
    connect(schoolLabelWidget, &TACSchoolLabelWidget::doubleClicked, this, [=]() {
        logoDialog->show();
    });
    schoolLabelWidget->show();

    classLabelWidget = new TACClassLabelWidget(this);
    connect(classLabelWidget, &TACClassLabelWidget::doubleClicked, this, [=]() {
        logoDialog->show();
    });
    classLabelWidget->show();

    trayLabelWidget = new TACTrayLabelWidget(this);
    trayLabelWidget->updateLogo(".\\res\\img\\com_bottom_ic_component@2x.png");
    //connect(trayLabelWidget, &TACTrayLabelWidget::doubleClicked, this, [=]() {
    //    logoDialog->show();
    //    });
    trayLabelWidget->show();

    connect(trayLabelWidget, &TACTrayLabelWidget::clicked, this, [=]() {
        //logoDialog->show();
        if (trayWidget)
        {
            QRect rect = trayLabelWidget->geometry();
            trayWidget->move(rect.x() - trayWidget->width(), rect.y() - trayWidget->height() - 10);
            if (trayWidget->isHidden())
                trayWidget->show();
            else
                trayWidget->hide();
        }
        });

    logoDialog = new TACLogoDialog(this);
    connect(logoDialog, &TACLogoDialog::enterClicked, this, [=]() {
        if (schoolLabelWidget && !logoDialog->getSchoolName().isEmpty())
        {
            schoolLabelWidget->setContent(logoDialog->getSchoolName());
        }
        if (classLabelWidget && !logoDialog->getClassName().isEmpty())
        {
            classLabelWidget->setContent(logoDialog->getClassName());
        }
        if (trayLabelWidget && !logoDialog->getClassName().isEmpty())
        {
            trayLabelWidget->setContent(logoDialog->getClassName());
        }
    });
   
    folderDialog = new TACFolderDialog(this);
    folderWidget = new TACFolderWidget(this);
    connect(folderWidget, &TACFolderWidget::clicked, this, [=]() {
        folderDialog->show();
    });
    folderWidget->show();

    courseSchedule = new TACCourseSchedule(this);
    courseSchedule->show();

    QMap<TimeRange, QString> classMap;
    classMap[TimeRange(QTime(8, 0), QTime(8, 45))] = "数学";
    classMap[TimeRange(QTime(8, 45), QTime(9, 30))] = "语文";
    classMap[TimeRange(QTime(9, 30), QTime(10, 15))] = "大课间";
    classMap[TimeRange(QTime(10, 30), QTime(11, 30))] = "英语";
    classMap[TimeRange(QTime(13, 0), QTime(14, 0))] = "午休";
    classMap[TimeRange(QTime(14, 15), QTime(15, 0))] = "历史";

    courseSchedule->updateClass(classMap);
    classWeekCourseScheduldDialog = new TACClassWeekCourseScheduleDialog(this);

    if (m_httpHandler)
    {
        //QMap<QString, QString> params;
        //params["phone"] = "13621907363";
        m_httpHandler->get(QString("http://47.100.126.194:5000/userInfo?phone=" + qPhone));
    }
    //updateBackground("C:/workspace/obs/TeacherAssistant/TeacherAssistantClient/res/bg/5.jpg");
}
void TACMainDialog::updateBackground(const QString & fileName)
{
    m_background = QPixmap(fileName);
    update();
}
void TACMainDialog::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (m_background.isNull()) {
        return;
    }
    QPixmap scaled = m_background.scaled(
        size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
    );
    QRect r = scaled.rect();
    r.moveCenter(rect().center());
    painter.drawPixmap(r, scaled);
}

void TACMainDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    this->update();
}