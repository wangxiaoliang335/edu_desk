#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QIcon>
#include <qpainterpath.h>
#include <qtoolbutton.h>
#include <QJsonObject>    // JSON 对象
#include <QJsonArray>     // JSON 数组
#include <QJsonDocument>  // JSON 文档（序列化/反序列化）
#include <QJsonValue>     // JSON 值类型
#include <qdebug.h>
#include "TABaseDialog.h"
#include "AvatarLabel.h"
#include "NameLabel.h"
#include "TAHttpHandler.h"

class UserInfoDialog : public QDialog
{
    Q_OBJECT
public:
    UserInfoDialog(QWidget* parent = nullptr) : QDialog(parent),
        m_backgroundColor(QColor(50, 50, 50)),
        m_dragging(false),
        m_borderColor(Qt::white),
        m_borderWidth(2), m_radius(6),
        m_visibleCloseButton(true)
    {
        this->setObjectName("UserInfoDialog");
        m_httpHandler = new TAHttpHandler(this);
    }

    void InitData(UserInfo userInfo)
    {
        m_userInfo = userInfo;
    }

	void uploadAvatar(QString filePath)
	{
		// ===== 1. 读取头像图片 =====
		QFile file(filePath);  // 本地头像路径
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "Failed to open image file.";
			return;
		}
		QByteArray imageData = file.readAll(); // 二进制数据
		file.close();

		// ===== 2. 图片转 Base64 =====
		QString imageBase64 = QString::fromLatin1(imageData.toBase64());

        // ===== 3. 构造 JSON 数据 =====
        QMap<QString, QString> params;
        params["avatar"] = imageBase64;
        params["phone"] = m_userInfo.strPhone;
        params["name"] = m_userInfo.strName;
        params["sex"] = m_userInfo.strSex;
        params["address"] = m_userInfo.strAddress;
        params["school_name"] = m_userInfo.strSchoolName;
        params["grade_level"] = m_userInfo.strGradeLevel;
        params["grade"] = m_userInfo.strGrade;
        params["subject"] = m_userInfo.strSubject;
        params["class_taught"] = m_userInfo.strClassTaught;
        params["is_administrator"] = m_userInfo.strIsAdministrator;
        params["id_number"] = m_userInfo.strIdNumber;
        if (m_httpHandler)
        {
            m_httpHandler->post(QString("http://47.100.126.194:5000/updateUserInfo"), params);
        }
	}

    void InitUI()
    {
        // 隐藏标题栏（图片里没有标题栏）
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(330, 360);

        setWindowTitle("我的");
        resize(350, 600);
        //setStyleSheet("background-color: #2a2a2a; color: white;");

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
                            /*if (oTmp["userinfo"].isArray())
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

                                    if (userMenuDlg)
                                    {
                                        userMenuDlg->InitData(m_userInfo);
                                        userMenuDlg->InitUI();
                                    }
                                }
                            }*/
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

        // 关闭按钮（右上角）
        closeButton = new QPushButton(this);
        closeButton->setIcon(QIcon(":/res/img/widget-close.png"));
        closeButton->setIconSize(QSize(22, 22));
        closeButton->setFixedSize(QSize(22, 22));
        closeButton->setStyleSheet("background: transparent;");
        closeButton->move(width() - 24, 4);
        closeButton->hide();
        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);

        // ===================== 顶部区域 =====================
        QHBoxLayout* closeLayout = new QHBoxLayout;
        QHBoxLayout* topLayout = new QHBoxLayout;

        // 头像标签
        avatarLabel = new AvatarLabel;
        avatarLabel->setFixedSize(64, 64);
        avatarLabel->setStyleSheet("background-color: gray; border-radius: 32px;");
        avatarLabel->setAvatar(QPixmap(m_userInfo.strHeadImagePath));
        avatarLabel->setEditIcon(QPixmap(".\\res\\img\\com_ic_edit_2@2x.png"));

        QObject::connect(avatarLabel, &AvatarLabel::editIconClicked, [&]() {
            QString file = QFileDialog::getOpenFileName(
                this, "选择新头像", "", "Images (*.png *.jpg *.jpeg *.bmp)");
            if (!file.isEmpty()) {
                avatarLabel->setAvatar(QPixmap(file));
                uploadAvatar(file);
            }
        });

        QObject::connect(avatarLabel, &AvatarLabel::avatarClicked, []() {
            qDebug() << "头像被点击";
        });

        //// 编辑头像按钮
        //QToolButton* editAvatarBtn = new QToolButton;
        //editAvatarBtn->setIcon(QIcon(".\\res\\img\\com_ic_edit_2@2x.png"));
        //editAvatarBtn->setToolTip("编辑头像");

        // 姓名标签
        nameLabel = new NameLabel;

        //// 编辑姓名按钮
        //QToolButton* editNameBtn = new QToolButton;
        //editNameBtn->setIcon(QIcon(".\\res\\img\\com_ic_edit_2@2x.png"));
        //editNameBtn->setToolTip("编辑姓名");

        // 顶部横向布局：头像+编辑按钮，姓名+编辑按钮
        //QHBoxLayout* avatarLayout = new QHBoxLayout;
        //avatarLayout->addWidget(avatarLabel);
        //avatarLayout->addWidget(editAvatarBtn);
        //avatarLayout->addStretch();

        nameLabel->setText(m_userInfo.strName);
        nameLabel->setStyleSheet("color: white; font-size: 15px;");
        nameLabel->setEditIcon(QPixmap(".\\res\\img\\com_ic_edit_2.png")); // 编辑图标

        QObject::connect(nameLabel, &NameLabel::editIconClicked, [=]() {
            qDebug() << "编辑图标被点击";
        });

        QObject::connect(nameLabel, &NameLabel::labelClicked, [=]() {
            qDebug() << "姓名标签被点击";
        });

        //QHBoxLayout* nameLayout = new QHBoxLayout;
        //nameLayout->addWidget(nameLabel);
        //nameLayout->addWidget(editNameBtn);
        //nameLayout->addStretch();

        //QLabel* titleLabel = new QLabel("我的");
        //titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");

        /*QLabel* lblNumber = new QLabel("2");
        lblNumber->setAlignment(Qt::AlignCenter);
        lblNumber->setFixedSize(30, 30);
        lblNumber->setStyleSheet("background-color: yellow; border-radius: 15px; color: red; font-weight: bold;");*/

        QPushButton* btnUpgrade = new QPushButton("升级为\n管理员");
        btnUpgrade->setStyleSheet("background-color: lightgreen; color: black; padding:6px 10px; border-radius:6px;");

        QPushButton* btnManager = new QPushButton("管理员");
        btnManager->setStyleSheet("background-color: yellow; color: black; padding:6px 10px; border-radius:6px;");

        QPushButton* btnLogout = new QPushButton("退出登录");
        btnLogout->setStyleSheet("background-color: gray; color: white; padding:6px 10px; border-radius:6px;");

        QVBoxLayout* pVBoxLayout = new QVBoxLayout();
        pVBoxLayout->addWidget(btnManager);
        pVBoxLayout->addWidget(btnLogout);

        QLabel* pLabel = new QLabel(this);
        pLabel->setFixedSize(QSize(22, 22));
        closeLayout->addWidget(pLabel);
        closeLayout->addStretch();
        closeLayout->addWidget(closeButton);
        //topLayout->addWidget(lblNumber);
        //topLayout->addWidget(titleLabel);
        //topLayout->addStretch();
        topLayout->addWidget(avatarLabel);  //头像
        topLayout->addWidget(nameLabel);    //姓名
        topLayout->addWidget(btnUpgrade);
        topLayout->addLayout(pVBoxLayout);
        //topLayout->addWidget(btnManager);
        //topLayout->addWidget(btnLogout);

        // ===================== 用户基本信息 =====================
        QGridLayout* infoLayout = new QGridLayout;
        infoLayout->setHorizontalSpacing(20);
        infoLayout->setVerticalSpacing(12);

        infoLayout->addWidget(new QLabel("性别"), 0, 0);
        infoLayout->addWidget(new QLabel(m_userInfo.strSex), 0, 1);
        infoLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 2);

        infoLayout->addWidget(new QLabel("地址"), 1, 0);
        infoLayout->addWidget(new QLabel(m_userInfo.strAddress), 1, 1);

        infoLayout->addWidget(new QLabel("学校名"), 2, 0);
        infoLayout->addWidget(new QLabel(m_userInfo.strSchoolName), 2, 1);

        infoLayout->addWidget(new QLabel("学段"), 3, 0);
        infoLayout->addWidget(new QLabel(m_userInfo.strGradeLevel), 3, 1);

        infoLayout->addWidget(new QLabel("年级"), 4, 0);
        infoLayout->addWidget(new QLabel(m_userInfo.strGrade), 4, 1);

        infoLayout->addWidget(new QLabel("任教学科"), 5, 0);
        infoLayout->addWidget(new QLabel(m_userInfo.strSubject), 5, 1);

        infoLayout->addWidget(new QLabel("任教学段"), 6, 0);
        infoLayout->addWidget(new QLabel(m_userInfo.strClassTaught), 6, 1);

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
        //QHBoxLayout* toolLayout = new QHBoxLayout;
        //QStringList toolNames = { "电话", "消息", "相册", "文档", "设置", "用户" };
        //QStringList icons = { ":/icons/phone.png", ":/icons/message.png", ":/icons/photo.png", ":/icons/doc.png", ":/icons/settings.png", ":/icons/user.png" };

        //for (int i = 0; i < toolNames.size(); ++i) {
        //    QPushButton* btn = new QPushButton;
        //    btn->setIcon(QIcon(icons[i])); // 图标文件需要在资源中准备好
        //    btn->setIconSize(QSize(24, 24));
        //    btn->setFlat(true);
        //    btn->setStyleSheet("QPushButton {border:none; color: white;} QPushButton:hover { color: lightblue; }");
        //    toolLayout->addWidget(btn);
        //    toolLayout->addStretch();
        //}

        // ===================== 主布局 =====================
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(closeLayout);
        mainLayout->addLayout(topLayout);
        mainLayout->addSpacing(10);
        mainLayout->addLayout(infoLayout);
        mainLayout->addSpacing(20);
        mainLayout->addLayout(btnBottomLayout);

        setLayout(mainLayout);
        //mainLayout->addStretch();
        //mainLayout->addLayout(toolLayout);

        // 连接信号
        //connect(btnCancel, &QPushButton::clicked, this, &TABaseDialog::reject);
        //connect(btnOk, &QPushButton::clicked, this, &TABaseDialog::accept);
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

    QString titleName() const { return m_titleName; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor borderColor() const { return m_borderColor; }
    int borderWidth() const { return m_borderWidth; }
    int radius() const { return m_radius; }

    void mousePressEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
        }
    }

    void leaveEvent(QEvent* event)
    {
        QDialog::leaveEvent(event);
        closeButton->hide();
    }

    void enterEvent(QEvent* event)
    {
        QDialog::enterEvent(event);
        if (m_visibleCloseButton)
            closeButton->show();
    }

    void resizeEvent(QResizeEvent* event)
    {
        QDialog::resizeEvent(event);
        //initShow();
        closeButton->move(this->width() - 22, 0);
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

    UserInfo m_userInfo;
    bool m_dragging;
    QPoint m_dragStartPos;
    QString m_titleName;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    bool m_visibleCloseButton;
    QPushButton* closeButton = NULL;
    AvatarLabel* avatarLabel = NULL;
    NameLabel* nameLabel = NULL;
    TAHttpHandler* m_httpHandler = NULL;
};
