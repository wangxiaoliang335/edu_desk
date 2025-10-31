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
{// 右侧表单
    QGridLayout* formLayout = new QGridLayout;
    //QLabel *lblSchool = new QLabel("学校名");
    QLabel* lblSchool = new QLabel(" 学校名");
    editSchool = new QLineEdit;
    QLabel* lblAddr = new QLabel(" 地址");
    editAddr = new QLineEdit;

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
                                    if (lblCode)
                                    {
                                        //if (schoolSub[0].is)
                                        {
                                            lblCode->setText(schoolSub["id"].toString());
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
                    if (lblCode)
                    {
                        lblCode->setText(obj["code"].toString());

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

    lblSchool->setStyleSheet("background-color:blue; color:white; font-size:20px;");
    editSchool->setStyleSheet("background-color:blue; color:white; font-size:20px;");
    formLayout->addWidget(lblSchool, 0, 0, 1, 1);
    formLayout->addWidget(editSchool, 0, 1, 1, 2);
    formLayout->addWidget(lblAddr, 1, 0, 1, 1);
    formLayout->addWidget(editAddr, 1, 1, 1, 2);

    lblAddr->setStyleSheet("font-size:20px;");
    editAddr->setStyleSheet("font-size:20px;");

    errLabel = new QLabel(NULL, this);
    errLabel->setStyleSheet("background-color: rgba(255,255,255,200); font-size:16px; font-weight:bold;");
    errLabel->hide();

    // 右侧按钮区域
    QPushButton* btnGetCode = new QPushButton("获取组织代码");
    btnGetCode->setStyleSheet("background-color:green; color:white; font-size:16px;");
    lblCode = new QLabel();
    lblCode->setAlignment(Qt::AlignCenter);
    lblCode->setStyleSheet("background-color:blue; color:white; font-size:20px;");
    lblCode->setFixedSize(100, 40);

    QHBoxLayout* codeLayout = new QHBoxLayout;
    codeLayout->addWidget(btnGetCode);
    codeLayout->addWidget(lblCode);

    connect(btnGetCode, &QPushButton::clicked, this, [=]() {
        //UniqueNumberGenerator gen;
        //lblCode->setText(QString::number(gen.generate()));

        QUrl url("http://47.100.126.194:5000/unique6digit"); // MySQL+Redis服务端
        if (lblCode->text().isEmpty() && m_httpHandler)
        {
            m_httpHandler->get("http://47.100.126.194:5000/unique6digit");
        }
     });

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    QPushButton* pBtnConfirm = new QPushButton("应用");
    //QPushButton* pBtnCancel = new QPushButton("取消");
    bottomLayout->addWidget(errLabel);
    bottomLayout->addStretch(2);
    bottomLayout->addWidget(pBtnConfirm);
    bottomLayout->addStretch();
    //bottomLayout->addWidget(pBtnCancel);
    //bottomLayout->addStretch();

    pBtnConfirm->setStyleSheet("background-color:blue; color:white; font-size:20px;");
    connect(pBtnConfirm, &QPushButton::clicked, this, [=]() {
        //UniqueNumberGenerator gen;
        //lblCode->setText(QString::number(gen.generate()));
    });

    //connect(pBtnCancel, &QPushButton::clicked, this, [=]() {
    //    //UniqueNumberGenerator gen;
    //    //lblCode->setText(QString::number(gen.generate()));
    //});

    QVBoxLayout* rightLayout = new QVBoxLayout(this);
    rightLayout->addLayout(formLayout);
    rightLayout->addLayout(codeLayout);
    rightLayout->addStretch();
    rightLayout->addLayout(bottomLayout);
    setStyleSheet("background-color: rgba(255,255,255,200);");
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
    if (lblCode)
    {
        return lblCode->text();
    }
    return QString("");
}

