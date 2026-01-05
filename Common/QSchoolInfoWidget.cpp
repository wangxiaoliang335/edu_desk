#pragma execution_character_set("utf-8")
#include "QSchoolInfoWidget.h"
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "UniqueNumberGenerator.h"

QSchoolInfoWidget::QSchoolInfoWidget(QWidget *parent)
	: QWidget(parent)
{
    // 深色主题表单布局
    QVBoxLayout* formLayout = new QVBoxLayout;
    formLayout->setContentsMargins(0, 20, 0, 0);
    formLayout->setSpacing(24);
    
    // 学校名字段
    QLabel* lblSchool = new QLabel("学校名");
    lblSchool->setStyleSheet("color: rgba(255, 255, 255, 0.9); font-size: 14px; font-weight: 500;");
    editSchool = new QLineEdit;
    editSchool->setStyleSheet(
        "QLineEdit { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 14px; "
        "border: 1px solid rgba(255, 255, 255, 0.2); "
        "border-radius: 6px; "
        "padding: 8px 12px; "
        "} "
        "QLineEdit:focus { "
        "border-color: #2563eb; "
        "}");
    editSchool->setFixedHeight(40);
    
    // 地址字段
    QLabel* lblAddr = new QLabel("地址");
    lblAddr->setStyleSheet("color: rgba(255, 255, 255, 0.9); font-size: 14px; font-weight: 500;");
    editAddr = new QLineEdit;
    editAddr->setStyleSheet(
        "QLineEdit { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 14px; "
        "border: 1px solid rgba(255, 255, 255, 0.2); "
        "border-radius: 6px; "
        "padding: 8px 12px; "
        "} "
        "QLineEdit:focus { "
        "border-color: #2563eb; "
        "}");
    editAddr->setFixedHeight(40);

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
                    //QString strUserId = oTmp["user_id"].toString();
                    qDebug() << "status:" << oTmp["code"].toString();
                    qDebug() << "msg:" << oTmp["message"].toString(); // 如果 msg 是中文，也能正常输出
                    if (strTmp == "获取学校列表成功")
                    {
                        if (oTmp["schools"].isArray())
                        {
                            QJsonArray schoolArr = oTmp["schools"].toArray();
                            if (schoolArr.size() > 0)
                            {
                                if (schoolArr[0].isObject())
                                {
                                    QJsonObject schoolSub = schoolArr[0].toObject();
                                    if (editCode)
                                    {
                                        {
                                            editCode->setText(schoolSub["id"].toString());
                                        }
                                        /*else if (schoolSub[0].toString())
                                        {

                                        }*/
                                    }
                                }
                            }
                        }
                    }
                    //errLabel->setText(strTmp);
                }
                else
                {
                    if (editCode)
                    {
                        editCode->setText(obj["code"].toString());

                        QMap<QString, QString> params;
                        params["id"] = obj["code"].toString();
                        params["name"] = m_userInfo.strSchoolName;
                        params["address"] = m_userInfo.strAddress;
                        m_httpHandler->post(QString("http://47.100.126.194:5000/updateSchoolInfo"), params);
                    }
                }
            }
            else
            {
                errLabel->setText("网络错误");
                errLabel->show();
            }
            });

        connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
            if (errLabel)
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
                        errLabel->setText(strTmp);
                        errLabel->show();
                    }
                }
                else
                {
                    errLabel->setText("网络错误");
                    errLabel->show();
                }
            }
            });
    }

    // 组织代码字段 - 标签 + 输入框 + 按钮在同一行
    QLabel* lblCodeLabel = new QLabel("组织代码");
    lblCodeLabel->setStyleSheet("color: rgba(255, 255, 255, 0.9); font-size: 14px; font-weight: 500;");
    
    editCode = new QLineEdit;
    editCode->setStyleSheet(
        "QLineEdit { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 14px; "
        "border: 1px solid rgba(255, 255, 255, 0.2); "
        "border-radius: 6px; "
        "padding: 8px 12px; "
        "} "
        "QLineEdit:focus { "
        "border-color: #2563eb; "
        "}");
    editCode->setFixedHeight(40);
    editCode->setReadOnly(true); // 组织代码只读，通过按钮获取
    editCode->setMinimumWidth(200);
    
    QPushButton* btnGetCode = new QPushButton("获取组织代码");
    btnGetCode->setFixedHeight(40);
    btnGetCode->setStyleSheet(
        "QPushButton { "
        "background-color: rgba(255, 255, 255, 0.1); "
        "color: white; "
        "font-size: 14px; "
        "border: 1px solid rgba(255, 255, 255, 0.2); "
        "border-radius: 6px; "
        "padding: 8px 16px; "
        "} "
        "QPushButton:hover { "
        "background-color: rgba(255, 255, 255, 0.15); "
        "}");

    // 组织代码布局：输入框 + 按钮
    QHBoxLayout* codeInputLayout = new QHBoxLayout;
    codeInputLayout->setSpacing(12);
    codeInputLayout->addWidget(editCode);
    codeInputLayout->addWidget(btnGetCode);
    codeInputLayout->addStretch();

    // 错误提示标签
    errLabel = new QLabel(this);
    errLabel->setStyleSheet("color: #ef4444; font-size: 14px; font-weight: 500;");
    errLabel->hide();
    
    connect(btnGetCode, &QPushButton::clicked, this, [=]() {
        QUrl url("http://47.100.126.194:5000/unique6digit"); // MySQL+Redis服务端
        if (editCode && editCode->text().isEmpty() && m_httpHandler)
        {
            m_httpHandler->get("http://47.100.126.194:5000/unique6digit");
        }
     });

    // 表单布局：垂直排列各个字段
    formLayout->addWidget(lblSchool);
    formLayout->addWidget(editSchool);
    formLayout->addWidget(lblAddr);
    formLayout->addWidget(editAddr);
    formLayout->addWidget(lblCodeLabel);
    formLayout->addLayout(codeInputLayout);
    formLayout->addWidget(errLabel);
    formLayout->addStretch();

    // 主布局
    QVBoxLayout* rightLayout = new QVBoxLayout(this);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addLayout(formLayout);
    
    // 设置透明背景，让父窗口的背景显示
    setStyleSheet("QWidget { background-color: transparent; }");
}

QSchoolInfoWidget::~QSchoolInfoWidget()
{}

void QSchoolInfoWidget::InitData(UserInfo userInfo)
{
    m_userInfo = userInfo;
    if (editSchool)
    {
        editSchool->setText(m_userInfo.strSchoolName);
    }
    if (editAddr)
    {
        editAddr->setText(m_userInfo.strAddress);
    }

    if (m_httpHandler)
    {
        QString qUrl("http://47.100.126.194:5000/schools?name=");
        qUrl += m_userInfo.strSchoolName;
        m_httpHandler->get(qUrl);
    }
}

QString QSchoolInfoWidget::getSchoolId()
{
    if (editCode)
    {
        return editCode->text();
    }
    return QString("");
}

