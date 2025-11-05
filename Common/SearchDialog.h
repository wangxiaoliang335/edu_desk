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
#include "TAHttpHandler.h"
#include "CommonInfo.h"

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

private slots:
    void onSearchClicked()
    {
        QString searchKey = m_editSearch->text().trimmed();
        if (searchKey.isEmpty()) {
            return;
        }

        // 获取当前用户信息
        UserInfo userInfo = CommonInfo::GetData();
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

                // 添加搜索结果项
                addListItem(m_listLayout, "", groupName, QString::number(memberNum), 
                           classid.isEmpty() ? "群组" : "班级群", desc, groupId);
            }
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

private:
    void addListItem(QVBoxLayout* parent, const QString& iconPath,
        const QString& name, const QString& memberCount,
        const QString& tag, const QString& desc, const QString& groupId = "")
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

        QPushButton* btnJoin = new QPushButton("加入");
        btnJoin->setStyleSheet("background-color: lightblue; padding: 4px 8px;");
        btnJoin->setProperty("group_id", groupId);
        btnJoin->setProperty("group_name", name);

        // 连接加入按钮点击事件
        connect(btnJoin, &QPushButton::clicked, this, [=]() {
            qDebug() << "加入群组:" << groupId << name;
            // TODO: 实现加入群组的逻辑
        });

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
};
