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
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    SearchDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("查找班级/教师/群");
        resize(500, 600);

        // 初始化HTTP处理器
        m_httpHandler = new TAHttpHandler(this);
        if (m_httpHandler)
        {
            connect(m_httpHandler, &TAHttpHandler::success, this, [=](const QString& responseString) {
                handleSearchResponse(responseString);
            });

            connect(m_httpHandler, &TAHttpHandler::failed, this, [=](const QString& errResponseString) {
                QJsonDocument jsonDoc = QJsonDocument::fromJson(errResponseString.toUtf8());
                if (jsonDoc.isObject()) {
                    QJsonObject obj = jsonDoc.object();
                    if (obj["data"].isObject()) {
                        QJsonObject dataObj = obj["data"].toObject();
                        QString message = dataObj["message"].toString();
                        qDebug() << "搜索失败:" << message;
                    }
                }
            });
        }

        // 整体布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 顶部数字标签
        m_lblNum = new QLabel("0");
        m_lblNum->setAlignment(Qt::AlignCenter);
        m_lblNum->setFixedSize(30, 30);
        m_lblNum->setStyleSheet("background-color: yellow; color: red; font-weight: bold; font-size: 16px; border-radius: 15px;");
        mainLayout->addWidget(m_lblNum, 0, Qt::AlignCenter);

        // 搜索栏
        QHBoxLayout* searchLayout = new QHBoxLayout;
        m_editSearch = new QLineEdit;
        m_editSearch->setPlaceholderText("输入群ID或群名称");
        QPushButton* btnSearch = new QPushButton("搜索");
        btnSearch->setStyleSheet("background-color: blue; color: white; padding: 6px 12px;");
        searchLayout->addWidget(m_editSearch);
        searchLayout->addWidget(btnSearch);
        mainLayout->addLayout(searchLayout);

        // 连接搜索按钮点击事件
        connect(btnSearch, &QPushButton::clicked, this, &SearchDialog::onSearchClicked);
        // 连接回车键搜索
        connect(m_editSearch, &QLineEdit::returnPressed, this, &SearchDialog::onSearchClicked);

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
        m_scrollArea = new QScrollArea;
        m_scrollArea->setWidgetResizable(true);
        m_listContainer = new QWidget;
        m_listLayout = new QVBoxLayout(m_listContainer);
        m_listLayout->addStretch();
        m_scrollArea->setWidget(m_listContainer);
        mainLayout->addWidget(m_scrollArea);
    }

signals:
    void groupJoined(const QString& groupId); // 加入群组成功信号

private slots:
    void onSearchClicked()
    {
        // 获取当前用户信息
        //UserInfo userInfo = CommonInfo::GetData();
        // 获取已加入的群组列表（用于判断是否已加入）
        loadJoinedGroups();

        QString searchKey = m_editSearch->text().trimmed();
        if (searchKey.isEmpty()) {
            return;
        }

        // 获取当前用户信息
        UserInfo userInfo = CommonInfo::GetData();
        m_currentUserId = userInfo.teacher_unique_id;
        if (userInfo.schoolId.isEmpty()) {
            qDebug() << "学校ID为空，无法搜索";
            return;
        }

        // 判断搜索类型：如果搜索键是纯数字或包含@TGS，可能是group_id；否则可能是group_name
        QString searchType = "group_name"; // 默认按名称搜索
        if (searchKey.contains("@TGS") || (searchKey.length() <= 10 && searchKey.toInt() > 0)) {
            searchType = "group_id";
        }

        // 构建URL
        QUrl url("http://47.100.126.194:5000/groups/search");
        QUrlQuery query;
        query.addQueryItem("schoolid", userInfo.schoolId);
        if (searchType == "group_id") {
            query.addQueryItem("group_id", searchKey);
        } else {
            query.addQueryItem("group_name", searchKey);
        }
        url.setQuery(query);

        // 发送搜索请求
        if (m_httpHandler) {
            m_httpHandler->get(url.toString());
            qDebug() << "搜索请求:" << url.toString();
        }
    }

    void handleSearchResponse(const QString& responseString)
    {
        // 清空之前的搜索结果
        clearSearchResults();

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseString.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON解析错误:" << parseError.errorString();
            return;
        }

        if (!jsonDoc.isObject()) {
            qDebug() << "返回的不是JSON对象";
            return;
        }

        QJsonObject obj = jsonDoc.object();
        if (!obj["data"].isObject()) {
            qDebug() << "返回数据中没有data字段";
            return;
        }

        QJsonObject dataObj = obj["data"].toObject();
        
        // 更新搜索结果数量
        int count = dataObj["count"].toInt();
        m_lblNum->setText(QString::number(count));

        // 处理群组列表
        if (dataObj["groups"].isArray()) {
            QJsonArray groupsArray = dataObj["groups"].toArray();
            for (int i = 0; i < groupsArray.size(); i++) {
                QJsonObject groupObj = groupsArray[i].toObject();
                QString groupId = groupObj["group_id"].toString();
                QString groupName = groupObj["group_name"].toString();
                QString classid = groupObj["classid"].toString();
                QString schoolid = groupObj["schoolid"].toString();
                QString introduction = groupObj["introduction"].toString();
                QString notification = groupObj["notification"].toString();
                int memberNum = groupObj["member_num"].toInt();

                // 构建描述信息
                QString desc = introduction.isEmpty() ? notification : introduction;
                if (desc.isEmpty()) {
                    desc = "群组ID: " + groupId;
                }

                // 判断当前用户是否已经是该群的成员
                bool isMember = m_joinedGroupIds.contains(groupId);

                // 添加搜索结果项
                addListItem(m_listLayout, "", groupName, QString::number(memberNum), 
                           classid.isEmpty() ? "群组" : "班级群", desc, groupId, isMember);
            }
        }
    }

    void loadJoinedGroups()
    {
        // 通过/groups/by-teacher接口获取当前用户的群组列表
        UserInfo userInfo = CommonInfo::GetData();
        if (m_httpHandler && !userInfo.teacher_unique_id.isEmpty())
        {
            QUrl url("http://47.100.126.194:5000/groups/by-teacher");
            QUrlQuery query;
            query.addQueryItem("teacher_unique_id", userInfo.teacher_unique_id);
            url.setQuery(query);
            
            // 使用临时标志来区分这个请求是用于加载已加入群组列表的
            // 由于TAHttpHandler的success信号会触发handleSearchResponse，我们需要区分
            // 这里直接使用QNetworkAccessManager来避免冲突
            QNetworkAccessManager* tempManager = new QNetworkAccessManager(this);
            QNetworkRequest request(url);
            QNetworkReply* reply = tempManager->get(request);
            
            connect(reply, &QNetworkReply::finished, this, [=]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray data = reply->readAll();
                    QJsonParseError parseError;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);
                    if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
                        QJsonObject obj = jsonDoc.object();
                        if (obj["data"].isObject()) {
                            QJsonObject dataObj = obj["data"].toObject();
                            
                            // 处理群主群组列表
                            if (dataObj["owner_groups"].isArray()) {
                                QJsonArray ownerGroups = dataObj["owner_groups"].toArray();
                                for (int i = 0; i < ownerGroups.size(); i++) {
                                    QJsonObject group = ownerGroups[i].toObject();
                                    QString groupId = group["group_id"].toString();
                                    if (!groupId.isEmpty()) {
                                        m_joinedGroupIds.insert(groupId);
                                    }
                                }
                            }
                            
                            // 处理成员群组列表
                            if (dataObj["member_groups"].isArray()) {
                                QJsonArray memberGroups = dataObj["member_groups"].toArray();
                                for (int i = 0; i < memberGroups.size(); i++) {
                                    QJsonObject group = memberGroups[i].toObject();
                                    QString groupId = group["group_id"].toString();
                                    if (!groupId.isEmpty()) {
                                        m_joinedGroupIds.insert(groupId);
                                    }
                                }
                            }
                            
                            qDebug() << "已加载已加入群组列表，共" << m_joinedGroupIds.size() << "个群组";
                        }
                    }
                } else {
                    qDebug() << "加载已加入群组列表失败:" << reply->errorString();
                }
                reply->deleteLater();
                tempManager->deleteLater();
            });
        }
    }

    void clearSearchResults()
    {
        // 清空列表容器中的所有子控件（保留最后的stretch）
        QLayoutItem* item;
        while ((item = m_listLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        m_listLayout->addStretch(); // 重新添加stretch
    }

private slots:
    void onJoinGroupClicked(const QString& groupId, const QString& groupName)
    {
        // 获取当前用户信息
        UserInfo userInfo = CommonInfo::GetData();
        QString userId = userInfo.teacher_unique_id; // 使用strUserId作为用户ID
        QString userName = userInfo.strName;
        
        if (userId.isEmpty() || userName.isEmpty()) {
            QMessageBox::warning(this, "错误", "用户信息不完整，无法加入群组！");
            return;
        }
        
        // 弹出输入框让用户输入加入理由
        bool ok;
        QString reason = QInputDialog::getText(this, "申请加入群组", 
            QString("请输入加入群组 \"%1\" 的理由：").arg(groupName),
            QLineEdit::Normal, "", &ok);
        
        if (!ok || reason.isEmpty()) {
            return; // 用户取消或未输入理由
        }
        
        // 创建一个结构体来保存回调需要的数据
        struct JoinGroupCallbackData {
            SearchDialog* dlg;
            QString groupId;
            QString groupName;
            QString userId;
            QString userName;
            QString reason;
        };
        
        JoinGroupCallbackData* callbackData = new JoinGroupCallbackData;
        callbackData->dlg = this;
        callbackData->groupId = groupId;
        callbackData->groupName = groupName;
        callbackData->userId = userId;
        callbackData->userName = userName;
        callbackData->reason = reason;
        
        // 先调用腾讯SDK的申请加入群组接口
        QByteArray groupIdBytes = groupId.toUtf8();
        QByteArray reasonBytes = reason.toUtf8();
        
        int ret = TIMGroupJoin(groupIdBytes.constData(), reasonBytes.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                JoinGroupCallbackData* data = (JoinGroupCallbackData*)user_data;
                SearchDialog* dlg = data->dlg;
                
                if (code == TIM_SUCC) {
                    qDebug() << "腾讯SDK申请加入群组成功:" << data->groupId;
                    
                    // 腾讯SDK成功，现在调用自己的服务器接口
                    dlg->sendJoinGroupRequestToServer(data->groupId, data->userId, data->userName, data->reason);
                    
                    // 显示成功消息
                    QMessageBox::information(dlg, "申请成功", 
                        QString("已成功申请加入群组 \"%1\"！\n等待管理员审核。").arg(data->groupName));
                } else {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    QString errorMsg = QString("申请加入群组失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc);
                    qDebug() << errorMsg;
                    QMessageBox::critical(dlg, "申请失败", errorMsg);
                }
                
                // 释放回调数据
                delete data;
            }, callbackData);
        
        if (ret != TIM_SUCC) {
            QString errorMsg = QString("调用TIMGroupJoin接口失败，错误码: %1").arg(ret);
            qDebug() << errorMsg;
            QMessageBox::critical(this, "错误", errorMsg);
            delete callbackData;
        }
    }
    
    void sendJoinGroupRequestToServer(const QString& groupId, const QString& userId, 
                                     const QString& userName, const QString& reason)
    {
        // 构造发送到服务器的JSON数据
        QJsonObject requestData;
        requestData["group_id"] = groupId;
        requestData["user_id"] = userId;
        requestData["user_name"] = userName;
        requestData["reason"] = reason;
        
        // 转换为JSON字符串
        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 发送POST请求到服务器
        QString url = "http://47.100.126.194:5000/groups/join";
        
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
                        qDebug() << "加入群组请求已发送到服务器";
                        
                        // 服务器响应成功，发出加入群组成功信号，通知父窗口刷新群组列表
                        emit this->groupJoined(groupId);
                    } else {
                        QString message = obj["message"].toString();
                        qDebug() << "服务器返回错误:" << message;
                    }
                }
            } else {
                qDebug() << "发送加入群组请求到服务器失败:" << reply->errorString();
            }
            
            reply->deleteLater();
            manager->deleteLater();
        });
    }

private:
    void addListItem(QVBoxLayout* parent, const QString& iconPath,
        const QString& name, const QString& memberCount,
        const QString& tag, const QString& desc, const QString& groupId = "", bool isMember = false)
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

        QPushButton* btnJoin = new QPushButton(isMember ? "已加入" : "加入");
        
        // 根据是否已加入设置按钮样式和状态
        if (isMember) {
            // 已加入：灰化按钮，禁用
            btnJoin->setEnabled(false);
            btnJoin->setStyleSheet("background-color: gray; color: white; padding: 4px 8px;");
        } else {
            // 未加入：可点击
            btnJoin->setEnabled(true);
        btnJoin->setStyleSheet("background-color: lightblue; padding: 4px 8px;");
            
            // 连接加入按钮点击事件
            connect(btnJoin, &QPushButton::clicked, this, [=]() {
                onJoinGroupClicked(groupId, name);
            });
        }
        
        btnJoin->setProperty("group_id", groupId);
        btnJoin->setProperty("group_name", name);

        itemLayout->addWidget(avatar);
        itemLayout->addLayout(infoLayout);
        itemLayout->addStretch();
        itemLayout->addWidget(btnJoin);

        QFrame* frame = new QFrame;
        frame->setLayout(itemLayout);
        frame->setFrameShape(QFrame::HLine);
        parent->insertWidget(parent->count() - 1, frame); // 在stretch之前插入
    }

private:
    TAHttpHandler* m_httpHandler = nullptr;
    QLineEdit* m_editSearch = nullptr;
    QLabel* m_lblNum = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_listContainer = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QString m_currentUserId; // 当前用户ID
    QSet<QString> m_joinedGroupIds; // 已加入的群组ID集合
};
