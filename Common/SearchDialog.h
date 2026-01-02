#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QButtonGroup>
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
#include <QMouseEvent>
#include <QPoint>
#include <QEvent>
#include <QResizeEvent>
#include "TAHttpHandler.h"
#include "CommonInfo.h"
#include "TaQTWebSocket.h"
#include "ImSDK/includes/TIMCloud.h"
#include "ImSDK/includes/TIMCloudDef.h"

// 自定义输入对话框类（类似SearchDialog样式，无标题栏）
class SearchInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchInputDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setModal(true);
        resize(400, 180);
        
        // 设置窗口背景色和圆角
        setStyleSheet(
            "QDialog { background-color: #565656; border-radius: 20px; } "
            "QLabel { background-color: #565656; color: #f5f5f5; } "
            "QLineEdit { background-color: #454545; color: #f5f5f5; border-radius: 15px; padding: 6px; } "
            "QPushButton { background-color: #454545; color: #f5f5f5; border-radius: 15px; } "
        );
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);
        
        // 标题标签
        m_titleLabel = new QLabel("输入", this);
        m_titleLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 18px; font-weight: bold;");
        m_titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_titleLabel);
        
        // 提示标签
        m_promptLabel = new QLabel("", this);
        m_promptLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 14px;");
        m_promptLabel->setWordWrap(true);
        mainLayout->addWidget(m_promptLabel);
        
        // 输入框
        m_lineEdit = new QLineEdit(this);
        m_lineEdit->setStyleSheet("background-color: #454545; color: #f5f5f5; border-radius: 15px; padding: 6px;");
        mainLayout->addWidget(m_lineEdit);
        
        // 按钮布局
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        
        m_okButton = new QPushButton("确定", this);
        m_okButton->setFixedSize(80, 35);
        m_okButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(m_okButton);
        
        m_cancelButton = new QPushButton("取消", this);
        m_cancelButton->setFixedSize(80, 35);
        m_cancelButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        buttonLayout->addWidget(m_cancelButton);
        
        buttonLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
        
        // 关闭按钮（鼠标移入显示）
        m_closeButton = new QPushButton("×", this);
        m_closeButton->setFixedSize(30, 30);
        m_closeButton->setStyleSheet(
            "QPushButton { "
            "background-color: #707070; "
            "color: #f5f5f5; "
            "border: none; "
            "border-radius: 15px; "
            "font-size: 20px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #ff4444; "
            "}"
        );
        m_closeButton->move(width() - 35, 5);
        m_closeButton->hide();
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
        
        // 初始化拖动
        m_dragging = false;
        m_dragStartPos = QPoint();
        
        // 连接回车键
        connect(m_lineEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
    }
    
    void setTitle(const QString& title) {
        if (m_titleLabel) {
            m_titleLabel->setText(title);
        }
    }
    
    void setPrompt(const QString& prompt) {
        if (m_promptLabel) {
            m_promptLabel->setText(prompt);
        }
    }
    
    void setText(const QString& text) {
        if (m_lineEdit) {
            m_lineEdit->setText(text);
        }
    }
    
    QString text() const {
        return m_lineEdit ? m_lineEdit->text() : QString();
    }
    
    static QString getText(QWidget* parent, const QString& title, const QString& prompt, 
                          const QString& defaultValue = "", bool* ok = nullptr)
    {
        SearchInputDialog dlg(parent);
        dlg.setTitle(title);
        dlg.setPrompt(prompt);
        dlg.setText(defaultValue);
        
        int result = dlg.exec();
        if (ok) {
            *ok = (result == QDialog::Accepted);
        }
        
        return result == QDialog::Accepted ? dlg.text() : QString();
    }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
            event->accept();
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }
    
    void enterEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->show();
        }
        QDialog::enterEvent(event);
    }
    
    void leaveEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->hide();
        }
        QDialog::leaveEvent(event);
    }
    
    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        if (m_closeButton) {
            m_closeButton->move(width() - 35, 5);
        }
    }
    
private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_promptLabel = nullptr;
    QLineEdit* m_lineEdit = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

// 自定义消息对话框类（类似SearchDialog样式）
class SearchMessageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchMessageDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setModal(true);
        resize(400, 200);
        
        // 设置窗口背景色和圆角
        setStyleSheet(
            "QDialog { background-color: #565656; border-radius: 20px; } "
            "QLabel { background-color: #565656; color: #f5f5f5; } "
            "QPushButton { background-color: #454545; color: #f5f5f5; border-radius: 15px; } "
        );
        
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);
        
        // 标题标签
        m_titleLabel = new QLabel("提示", this);
        m_titleLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 18px; font-weight: bold;");
        m_titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_titleLabel);
        
        // 消息标签
        m_messageLabel = new QLabel("", this);
        m_messageLabel->setStyleSheet("background-color: #565656; color: #f5f5f5; font-size: 14px;");
        m_messageLabel->setWordWrap(true);
        m_messageLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_messageLabel);
        
        // 确定按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        m_okButton = new QPushButton("确定", this);
        m_okButton->setFixedSize(100, 35);
        m_okButton->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px; font-weight: bold;");
        connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addStretch();
        mainLayout->addLayout(buttonLayout);
        
        // 关闭按钮（鼠标移入显示）
        m_closeButton = new QPushButton("×", this);
        m_closeButton->setFixedSize(30, 30);
        m_closeButton->setStyleSheet(
            "QPushButton { "
            "background-color: #707070; "
            "color: #f5f5f5; "
            "border: none; "
            "border-radius: 15px; "
            "font-size: 20px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #ff4444; "
            "}"
        );
        m_closeButton->move(width() - 35, 5);
        m_closeButton->hide();
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);
        
        // 初始化拖动
        m_dragging = false;
        m_dragStartPos = QPoint();
    }
    
    void setTitle(const QString& title) {
        if (m_titleLabel) {
            m_titleLabel->setText(title);
        }
    }
    
    void setMessage(const QString& message) {
        if (m_messageLabel) {
            m_messageLabel->setText(message);
        }
    }
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
            event->accept();
        }
    }
    
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }
    
    void enterEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->show();
        }
        QDialog::enterEvent(event);
    }
    
    void leaveEvent(QEvent* event) override {
        if (m_closeButton) {
            m_closeButton->hide();
        }
        QDialog::leaveEvent(event);
    }
    
    void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        if (m_closeButton) {
            m_closeButton->move(width() - 35, 5);
        }
    }
    
private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    SearchDialog(QWidget* parent = nullptr, TaQTWebSocket* pWs = nullptr) : QDialog(parent), m_pWs(pWs)
    {
        setWindowTitle("查找班级/教师/群");
        resize(500, 600);
        
        // 去掉标题栏
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        
        // 设置窗口背景色和圆角，以及所有控件的背景色
        setStyleSheet(
            "QDialog { background-color: #565656; border-radius: 20px; } "
            "QWidget { background-color: #565656; } "
            "QLabel { background-color: #565656; } "
            "QLineEdit { background-color: #565656; } "
            "QPushButton { background-color: #454545; } "
            "QScrollArea { background-color: #565656; } "
            "QFrame { background-color: #565656; } "
            "QScrollBar:vertical { background-color: #565656; } "
            "QScrollBar:horizontal { background-color: #565656; } "
            "QScrollBar::handle:vertical { background-color: #707070; } "
            "QScrollBar::handle:horizontal { background-color: #707070; } "
        );

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

        // 初始化拖动相关变量
        m_dragging = false;
        m_dragStartPos = QPoint();

        // 创建关闭按钮
        m_closeButton = new QPushButton("×", this);
        m_closeButton->setFixedSize(30, 30);
        m_closeButton->setStyleSheet(
            "QPushButton { "
            "background-color: #707070; "
            "color: #f5f5f5; "
            "border: none; "
            "border-radius: 15px; "
            "font-size: 20px; "
            "font-weight: bold; "
            "} "
            "QPushButton:hover { "
            "background-color: #ff4444; "
            "}"
        );
        m_closeButton->move(width() - 35, 5);
        m_closeButton->hide(); // 初始隐藏
        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);

        // 整体布局
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // 顶部数字标签
        m_lblNum = new QLabel("");
        m_lblNum->setAlignment(Qt::AlignCenter);
        m_lblNum->setFixedSize(30, 30);
        m_lblNum->setStyleSheet("background-color: #565656; color: #f5f5f5; font-weight: bold; font-size: 16px; border-radius: 15px;");
        mainLayout->addWidget(m_lblNum, 0, Qt::AlignCenter);

        // 搜索栏
        QHBoxLayout* searchLayout = new QHBoxLayout;
        m_editSearch = new QLineEdit;
        m_editSearch->setPlaceholderText("输入班级编号/教师编号/群编号");
        m_editSearch->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px; border-radius: 15px;");
        QPushButton* btnSearch = new QPushButton("搜索");
        btnSearch->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 6px 12px; border-radius: 15px;");
        searchLayout->addWidget(m_editSearch);
        searchLayout->addWidget(btnSearch);
        mainLayout->addLayout(searchLayout);

        // 连接搜索按钮点击事件
        connect(btnSearch, &QPushButton::clicked, this, &SearchDialog::onSearchClicked);
        // 连接回车键搜索
        connect(m_editSearch, &QLineEdit::returnPressed, this, &SearchDialog::onSearchClicked);

        // 分类按钮行
        QHBoxLayout* filterLayout = new QHBoxLayout;
        m_btnAll = new QPushButton("全部");
        m_btnClass = new QPushButton("班级");
        m_btnTeacher = new QPushButton("教师");
        m_btnGroup = new QPushButton("群");

        // 创建按钮组
        m_categoryButtonGroup = new QButtonGroup(this);
        m_categoryButtonGroup->setExclusive(true);
        m_categoryButtonGroup->addButton(m_btnAll, 0);
        m_categoryButtonGroup->addButton(m_btnClass, 1);
        m_categoryButtonGroup->addButton(m_btnTeacher, 2);
        m_categoryButtonGroup->addButton(m_btnGroup, 3);

        QString buttonStyle = "background-color: #454545; color: #f5f5f5; padding: 6px 12px; font-weight: bold; border-radius: 15px;";
        QString checkedButtonStyle = "background-color: #3569ff; color: #ffffff; padding: 6px 12px; font-weight: bold; border-radius: 15px;";
        
        m_btnAll->setCheckable(true);
        m_btnClass->setCheckable(true);
        m_btnTeacher->setCheckable(true);
        m_btnGroup->setCheckable(true);
        m_btnAll->setChecked(true); // 默认选中"全部"
        m_currentSearchCategory = 0;
        
        // 设置初始样式，默认选中"全部"按钮
        m_btnAll->setStyleSheet(checkedButtonStyle);
        m_btnClass->setStyleSheet(buttonStyle);
        m_btnTeacher->setStyleSheet(buttonStyle);
        m_btnGroup->setStyleSheet(buttonStyle);
        
        // 连接分类按钮点击事件
        connect(m_categoryButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, [this](int id) {
            m_currentSearchCategory = id;
            // 更新按钮样式
            QString buttonStyle = "background-color: #454545; color: #f5f5f5; padding: 6px 12px; font-weight: bold; border-radius: 15px;";
            QString checkedButtonStyle = "background-color: #3569ff; color: #ffffff; padding: 6px 12px; font-weight: bold; border-radius: 15px;";
            m_btnAll->setStyleSheet(m_btnAll->isChecked() ? checkedButtonStyle : buttonStyle);
            m_btnClass->setStyleSheet(m_btnClass->isChecked() ? checkedButtonStyle : buttonStyle);
            m_btnTeacher->setStyleSheet(m_btnTeacher->isChecked() ? checkedButtonStyle : buttonStyle);
            m_btnGroup->setStyleSheet(m_btnGroup->isChecked() ? checkedButtonStyle : buttonStyle);
        });

        filterLayout->addWidget(m_btnAll);
        filterLayout->addWidget(m_btnClass);
        filterLayout->addWidget(m_btnTeacher);
        filterLayout->addWidget(m_btnGroup);
        mainLayout->addLayout(filterLayout);

        // 列表区域
        m_scrollArea = new QScrollArea;
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setStyleSheet("QScrollArea { background-color: #565656; }");
        m_listContainer = new QWidget;
        m_listContainer->setStyleSheet("QWidget { background-color: #565656; }");
        m_listLayout = new QVBoxLayout(m_listContainer);
        m_listLayout->addStretch();
        m_scrollArea->setWidget(m_listContainer);
        mainLayout->addWidget(m_scrollArea);
        
        // 注册接收 WebSocket 消息并连接信号
        // 连接 WebSocket 消息信号
        if (m_pWs) {
            TaQTWebSocket::regRecvDlg(this);
            connect(m_pWs, &TaQTWebSocket::newMessage, this, &SearchDialog::onWebSocketMessage);
        }
        // 我们需要在 onWebSocketMessage 中处理消息
    }

signals:
    void groupJoined(const QString& groupId); // 加入群组成功信号
    void classJoined(const QString& classCode); // 加入班级成功信号
    void friendAdded(const QString& teacherUniqueId); // 添加好友成功信号

private slots:
    void onWebSocketMessage(const QString& msg)
    {
        // 解析 WebSocket 消息
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            return; // 不是 JSON 格式，忽略
        }
        
        QJsonObject obj = jsonDoc.object();
        QString type = obj["type"].toString();
        
        // 处理添加好友成功消息
        if (type == "add_friend_success") {
            QString friendId = obj["friend_teacher_unique_id"].toString();
            QString message = obj["message"].toString();
            bool targetOnline = obj["target_online"].toBool();
            
            if (!friendId.isEmpty()) {
                // 添加到好友列表
                m_friendIds.insert(friendId);
                
                // 更新按钮状态
                if (m_addFriendButtons.contains(friendId)) {
                    QPushButton* btn = m_addFriendButtons[friendId];
                    if (btn) {
                        btn->setText("已添加");
                        btn->setEnabled(false);
                        btn->setStyleSheet("background-color: #454545; color: #d3d3d3; padding: 4px 12px; border-radius: 15px;");
                        // 断开之前的连接，因为按钮已经不可点击
                        btn->disconnect();
                    }
                }
                
                // 发出信号，通知 FriendGroupDialog 刷新好友列表
                emit this->friendAdded(friendId);
                
                qDebug() << "添加好友成功:" << friendId << "目标用户在线:" << targetOnline;
                
                // 显示成功提示
                SearchMessageDialog* msgDlg = new SearchMessageDialog(this);
                msgDlg->setTitle("成功");
                msgDlg->setMessage(message);
                msgDlg->exec();
            }
        } else if (type == "error") {
            // 处理错误消息（可能是添加好友失败）
            QString errorMsg = obj["message"].toString();
            // 只处理与添加好友相关的错误
            if (errorMsg.contains("添加好友") || errorMsg.contains("teacher_unique_id") || errorMsg.contains("自己")) {
                qDebug() << "添加好友失败:" << errorMsg;
                
                SearchMessageDialog* msgDlg = new SearchMessageDialog(this);
                msgDlg->setTitle("错误");
                msgDlg->setMessage(errorMsg);
                msgDlg->exec();
            }
        }
    }
    
    void onSearchClicked()
    {
        QString keyword = m_editSearch->text().trimmed();
        if (keyword.isEmpty()) {
            clearSearchResults();
            return;
        }
        
        // 获取已加入的群组列表（用于判断是否已加入）
        loadJoinedGroups();
        // 获取好友列表（用于判断教师是否已经是好友）
        loadFriends();
        
        // 清空之前的搜索结果
        clearSearchResults();
        m_searchClassResults.clear();
        m_searchTeacherResults.clear();
        m_searchGroupResults.clear();
        m_pendingSearchRequests = 0;
        
        // 重置搜索结果计数
        m_groupCount = 0;
        m_teacherCount = 0;
        m_classCount = 0;
        
        // 根据选择的分类执行搜索
        if (m_currentSearchCategory == 0) {
            // 全部：搜索所有类型（群只搜索班级群）
            m_pendingSearchRequests = 3; // 班级、教师、班级群
            searchClasses(keyword);
            searchTeachers(keyword);
            searchGroups(keyword); // 只搜索班级群
        } else if (m_currentSearchCategory == 1) {
            // 班级
            m_pendingSearchRequests = 1;
            searchClasses(keyword);
        } else if (m_currentSearchCategory == 2) {
            // 教师
            m_pendingSearchRequests = 1;
            searchTeachers(keyword);
        } else if (m_currentSearchCategory == 3) {
            // 群（只搜索班级群）
            m_pendingSearchRequests = 1; // 只有班级群
            searchGroups(keyword);
        }
    }

    void handleSearchResponse(const QString& responseString)
    {
        // 这个方法可能不再使用，因为searchClassGroups已经直接调用onSearchGroupResult
        // 保留以防其他地方调用
        onSearchGroupResult(responseString, true); // 服务器接口返回的是班级群
    }
    
    // 搜索班级
    void searchClasses(const QString& keyword)
    {
        if (!m_httpHandler)
            return;
        
        UserInfo userInfo = CommonInfo::GetData();
        QUrl url("http://47.100.126.194:5000/classes/search");
        QUrlQuery query;
        
        // 添加班级编号（必填）
        query.addQueryItem("class_code", keyword.trimmed());
        
        // 添加学校ID（可选，如果存在）
        if (!userInfo.schoolId.isEmpty()) {
            query.addQueryItem("schoolid", userInfo.schoolId);
        }
        
        url.setQuery(query);
        
        // 创建临时TAHttpHandler用于搜索请求
        TAHttpHandler* searchHandler = new TAHttpHandler(this);
        connect(searchHandler, &TAHttpHandler::success, this, [this](const QString& response) {
            onSearchClassResult(response);
            sender()->deleteLater();
        });
        connect(searchHandler, &TAHttpHandler::failed, this, [this](const QString& error) {
            qDebug() << "搜索班级失败:" << error;
            m_pendingSearchRequests--;
            if (m_pendingSearchRequests <= 0) {
                displaySearchResults();
            }
            sender()->deleteLater();
        });
        
        searchHandler->get(url.toString());
    }
    
    // 搜索教师
    void searchTeachers(const QString& keyword)
    {
        if (!m_httpHandler)
            return;
        
        UserInfo userInfo = CommonInfo::GetData();
        QUrl url("http://47.100.126.194:5000/teachers/search");
        QUrlQuery query;
        
        // 判断输入的是teacher_unique_id还是姓名
        QString trimmedKeyword = keyword.trimmed();
        
        // 判断是否为ID格式
        bool containsChinese = false;
        for (const QChar& ch : trimmedKeyword) {
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
                containsChinese = true;
                break;
            }
        }
        
        bool looksLikeId = !containsChinese && 
                           trimmedKeyword.length() <= 50 &&
                           (trimmedKeyword.contains("-") || 
                            trimmedKeyword.contains("_") ||
                            (trimmedKeyword.length() <= 30 && !trimmedKeyword.contains(" ")));
        
        if (looksLikeId) {
            query.addQueryItem("teacher_unique_id", trimmedKeyword);
        } else {
            query.addQueryItem("name", trimmedKeyword);
        }
        
        url.setQuery(query);
        
        TAHttpHandler* searchHandler = new TAHttpHandler(this);
        connect(searchHandler, &TAHttpHandler::success, this, [this](const QString& response) {
            onSearchTeacherResult(response);
            sender()->deleteLater();
        });
        connect(searchHandler, &TAHttpHandler::failed, this, [this](const QString& error) {
            qDebug() << "搜索教师失败:" << error;
            m_pendingSearchRequests--;
            if (m_pendingSearchRequests <= 0) {
                displaySearchResults();
            }
            sender()->deleteLater();
        });
        
        searchHandler->get(url.toString());
    }
    
    // 搜索群
    void searchGroups(const QString& keyword)
    {
        QString trimmedKeyword = keyword.trimmed();
        if (trimmedKeyword.isEmpty()) {
            return;
        }
        
        // 只搜索班级群：使用服务器接口
        searchClassGroups(keyword);
    }
    
    // 搜索班级群（使用服务器接口）
    void searchClassGroups(const QString& keyword)
    {
        if (!m_httpHandler)
            return;
        
        UserInfo userInfo = CommonInfo::GetData();
        QUrl url("http://47.100.126.194:5000/groups/search");
        QUrlQuery query;
        
        // 判断输入的是group_id还是群名称
        QString trimmedKeyword = keyword.trimmed();
        
        // 判断是否为ID格式
        bool containsChinese = false;
        for (const QChar& ch : trimmedKeyword) {
            if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
                containsChinese = true;
                break;
            }
        }
        
        bool looksLikeId = !containsChinese && 
                           trimmedKeyword.length() <= 50 &&
                           (trimmedKeyword.contains("-") || 
                            trimmedKeyword.contains("_") ||
                            (trimmedKeyword.length() <= 30 && !trimmedKeyword.contains(" ")));
        
        if (looksLikeId) {
            query.addQueryItem("group_id", trimmedKeyword);
        } else {
            query.addQueryItem("group_name", trimmedKeyword);
        }
        
        url.setQuery(query);
        
        TAHttpHandler* searchHandler = new TAHttpHandler(this);
        connect(searchHandler, &TAHttpHandler::success, this, [this](const QString& response) {
            onSearchGroupResult(response, true); // true表示是班级群
            sender()->deleteLater();
        });
        connect(searchHandler, &TAHttpHandler::failed, this, [this](const QString& error) {
            qDebug() << "搜索班级群失败:" << error;
            m_pendingSearchRequests--;
            if (m_pendingSearchRequests <= 0) {
                displaySearchResults();
            }
            sender()->deleteLater();
        });
        
        searchHandler->get(url.toString());
    }
    
    
    // 处理班级搜索结果
    void onSearchClassResult(const QString& response)
    {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "解析班级搜索结果失败:" << parseError.errorString();
            m_pendingSearchRequests--;
            if (m_pendingSearchRequests <= 0) {
                displaySearchResults();
            }
            return;
        }
        
        QJsonObject obj = jsonDoc.object();
        if (obj.contains("data") && obj["data"].isObject()) {
            QJsonObject dataObj = obj["data"].toObject();
            if (dataObj.contains("classes") && dataObj["classes"].isArray()) {
                QJsonArray classesArray = dataObj["classes"].toArray();
                for (int i = 0; i < classesArray.size(); i++) {
                    m_searchClassResults.append(classesArray[i].toObject());
                }
                m_classCount = classesArray.size();
            }
        }
        
        m_pendingSearchRequests--;
        if (m_pendingSearchRequests <= 0) {
            displaySearchResults();
        }
    }
    
    // 处理教师搜索结果
    void onSearchTeacherResult(const QString& response)
    {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "解析教师搜索结果失败:" << parseError.errorString();
            m_pendingSearchRequests--;
            if (m_pendingSearchRequests <= 0) {
                displaySearchResults();
            }
            return;
        }
        
        QJsonObject obj = jsonDoc.object();
        // 服务器返回格式：{ "data": { "teachers": [...] } } 或 { "data": [...] }
        if (obj.contains("data")) {
            QJsonValue dataValue = obj["data"];
            QJsonArray teachersArray;
            
            if (dataValue.isArray()) {
                // 如果 data 是数组，直接使用
                teachersArray = dataValue.toArray();
            } else if (dataValue.isObject()) {
                // 如果 data 是对象，尝试获取 teachers 数组
                QJsonObject dataObj = dataValue.toObject();
                if (dataObj.contains("teachers") && dataObj["teachers"].isArray()) {
                    teachersArray = dataObj["teachers"].toArray();
                }
            }
            
            // 处理教师列表
            for (int i = 0; i < teachersArray.size(); i++) {
                m_searchTeacherResults.append(teachersArray[i].toObject());
            }
            m_teacherCount = teachersArray.size();
        }
        
        m_pendingSearchRequests--;
        if (m_pendingSearchRequests <= 0) {
            displaySearchResults();
        }
    }
    
    // 处理群搜索结果（服务器接口返回的班级群）
    void onSearchGroupResult(const QString& response, bool isClassGroup = false)
    {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "解析群搜索结果失败:" << parseError.errorString();
            m_pendingSearchRequests--;
            if (m_pendingSearchRequests <= 0) {
                displaySearchResults();
            }
            return;
        }
        
        QJsonObject obj = jsonDoc.object();
        if (obj.contains("data") && obj["data"].isObject()) {
            QJsonObject dataObj = obj["data"].toObject();
            if (dataObj.contains("groups") && dataObj["groups"].isArray()) {
                QJsonArray groupsArray = dataObj["groups"].toArray();
                for (int i = 0; i < groupsArray.size(); i++) {
                    m_searchGroupResults.append(groupsArray[i].toObject());
                }
                m_groupCount += groupsArray.size(); // 累加计数（因为还有普通群的结果）
            }
        }
        
        m_pendingSearchRequests--;
        if (m_pendingSearchRequests <= 0) {
            displaySearchResults();
        }
    }
    
    // 显示搜索结果
    void displaySearchResults()
    {
        clearSearchResults();
        
        UserInfo userInfo = CommonInfo::GetData();
        QString currentUserId = userInfo.teacher_unique_id;
        
        // 如果选择了特定分类，只显示对应结果
        if (m_currentSearchCategory == 1) {
            // 只显示班级结果
            for (const QJsonObject& item : m_searchClassResults) {
                QString name = item["class_name"].toString();
                QString code = item["class_code"].toString();
                if (name.isEmpty()) name = code;
                addListItem(m_listLayout, "", name, "1", "班级", "班级编号: " + code, code, false, "class");
            }
        } else if (m_currentSearchCategory == 2) {
            // 只显示教师结果（过滤掉自己）
            int displayedCount = 0;
            for (const QJsonObject& item : m_searchTeacherResults) {
                QString id = item["teacher_unique_id"].toString();
                // 过滤掉自己
                if (id == currentUserId) {
                    continue;
                }
                
                QString name = item["name"].toString();
                if (name.isEmpty()) name = id;
                bool isFriend = m_friendIds.contains(id);
                addListItem(m_listLayout, "", name, "1", "教师", "教师唯一ID: " + id, id, isFriend, "teacher");
                displayedCount++;
            }
            // 更新教师计数（排除自己）
            m_teacherCount = displayedCount;
        } else if (m_currentSearchCategory == 3) {
            // 只显示群结果
            for (const QJsonObject& item : m_searchGroupResults) {
                QString name = item["group_name"].toString();
                QString id = item["group_id"].toString();
                QString classid = item["classid"].toString();
                QString introduction = item["introduction"].toString();
                QString notification = item["notification"].toString();
                int memberNum = item["member_num"].toInt();
                bool isMember = m_joinedGroupIds.contains(id);
                
                QString desc = introduction.isEmpty() ? notification : introduction;
                if (desc.isEmpty()) {
                    desc = "群组ID: " + id;
                }
                
                addListItem(m_listLayout, "", name, QString::number(memberNum), 
                           classid.isEmpty() ? "群组" : "班级群", desc, id, isMember, "group");
            }
        } else {
            // 显示全部结果
            for (const QJsonObject& item : m_searchClassResults) {
                QString name = item["class_name"].toString();
                QString code = item["class_code"].toString();
                if (name.isEmpty()) name = code;
                addListItem(m_listLayout, "", name, "1", "班级", "班级编号: " + code, code, false, "class");
            }
            
            // 显示教师结果（过滤掉自己）
            int displayedTeacherCount = 0;
            for (const QJsonObject& item : m_searchTeacherResults) {
                QString id = item["teacher_unique_id"].toString();
                // 过滤掉自己
                if (id == currentUserId) {
                    continue;
                }
                
                QString name = item["name"].toString();
                if (name.isEmpty()) name = id;
                bool isFriend = m_friendIds.contains(id);
                addListItem(m_listLayout, "", name, "1", "教师", "教师唯一ID: " + id, id, isFriend, "teacher");
                displayedTeacherCount++;
            }
            // 更新教师计数（排除自己）
            m_teacherCount = displayedTeacherCount;
            for (const QJsonObject& item : m_searchGroupResults) {
                QString name = item["group_name"].toString();
                QString id = item["group_id"].toString();
                QString classid = item["classid"].toString();
                QString introduction = item["introduction"].toString();
                QString notification = item["notification"].toString();
                int memberNum = item["member_num"].toInt();
                bool isMember = m_joinedGroupIds.contains(id);
                
                QString desc = introduction.isEmpty() ? notification : introduction;
                if (desc.isEmpty()) {
                    desc = "群组ID: " + id;
                }
                
                addListItem(m_listLayout, "", name, QString::number(memberNum), 
                           classid.isEmpty() ? "群组" : "班级群", desc, id, isMember, "group");
            }
        }
        
        updateTotalCount();
    }
    
    // 加入班级
    void onJoinClassClicked(const QString& classCode)
    {
        if (!m_httpHandler || classCode.isEmpty())
            return;
        
        UserInfo userInfo = CommonInfo::GetData();
        if (userInfo.teacher_unique_id.isEmpty()) {
            qDebug() << "无法加入班级：教师唯一ID为空";
            return;
        }
        
        QJsonObject requestData;
        requestData["teacher_unique_id"] = userInfo.teacher_unique_id;
        requestData["class_code"] = classCode;
        requestData["role"] = "teacher";
        
        QJsonDocument doc(requestData);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QString url = "http://47.100.126.194:5000/teachers/classes/add";
        
        TAHttpHandler* addHandler = new TAHttpHandler(this);
        connect(addHandler, &TAHttpHandler::success, this, [this, classCode](const QString& response) {
            qDebug() << "加入班级成功:" << classCode;
            SearchMessageDialog* msgDlg = new SearchMessageDialog(this);
            msgDlg->setTitle("成功");
            msgDlg->setMessage("已成功加入班级");
            msgDlg->exec();
            // 发出加入班级成功信号
            emit this->classJoined(classCode);
            sender()->deleteLater();
        });
        connect(addHandler, &TAHttpHandler::failed, this, [this](const QString& error) {
            qDebug() << "加入班级失败:" << error;
            SearchMessageDialog* msgDlg = new SearchMessageDialog(this);
            msgDlg->setTitle("失败");
            msgDlg->setMessage("加入班级失败：" + error);
            msgDlg->exec();
            sender()->deleteLater();
        });
        
        addHandler->post(url, jsonData);
    }
    
    // 添加好友
    void onAddFriendClicked(const QString& teacherUniqueId)
    {
        if (teacherUniqueId.isEmpty())
            return;
        
        UserInfo userInfo = CommonInfo::GetData();
        QString currentUserId = userInfo.teacher_unique_id;
        
        // 不能添加自己为好友
        if (teacherUniqueId == currentUserId) {
            SearchMessageDialog* msgDlg = new SearchMessageDialog(this);
            msgDlg->setTitle("提示");
            msgDlg->setMessage("不能添加自己为好友");
            msgDlg->exec();
            return;
        }
        
        // 如果已经是好友，提示
        if (m_friendIds.contains(teacherUniqueId)) {
            SearchMessageDialog* msgDlg = new SearchMessageDialog(this);
            msgDlg->setTitle("提示");
            msgDlg->setMessage("该用户已经是您的好友");
            msgDlg->exec();
            return;
        }
        
        // 构造 JSON 消息体
        QJsonObject jsonMsg;
        jsonMsg["type"] = "1";
        jsonMsg["teacher_unique_id"] = teacherUniqueId;
        jsonMsg["text"] = "我想添加您为好友";
        
        QJsonDocument doc(jsonMsg);
        QString jsonString = doc.toJson(QJsonDocument::Compact);
        
        // 构造完整消息：to:<target_id>:<JSON消息>
        // target_id 是接收加好友请求的用户（被添加方的 teacher_unique_id）
        QString message = QString("to:%1:%2").arg(teacherUniqueId, jsonString);
        
        // 通过 WebSocket 发送消息
        TaQTWebSocket::sendPrivateMessage(message);
        
        qDebug() << "发送加好友请求:" << message;
    }


    void updateTotalCount()
    {
        // 更新总搜索结果数量
        int totalCount = m_groupCount + m_teacherCount + m_classCount;
        m_lblNum->setText(QString::number(totalCount));
    }

    void loadJoinedGroups()
    {
        // 1. 从腾讯IM获取已加入的普通群列表（Public类型）
        loadJoinedGroupsFromIM();
        
        // 2. 从服务器接口获取已加入的班级群列表
        loadJoinedClassGroupsFromServer();
    }
    
    // 从腾讯IM获取已加入的普通群列表
    void loadJoinedGroupsFromIM()
    {
        int ret = TIMGroupGetJoinedGroupList([](int32_t code, const char* desc, const char* json_param, const void* user_data) {
            SearchDialog* dlg = (SearchDialog*)user_data;
            if (!dlg) return;
            
            if (code != 0) {
                qDebug() << "获取已加入群组列表失败，错误码:" << code << "错误描述:" << (desc ? desc : "");
                return;
            }
            
            if (strlen(json_param) == 0) {
                return;
            }
            
            QJsonParseError parseError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(QByteArray(json_param), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "解析已加入群组列表失败:" << parseError.errorString();
                return;
            }
            
            if (!jsonDoc.isArray()) {
                qDebug() << "已加入群组列表不是数组格式";
                return;
            }
            
            QJsonArray json_group_list = jsonDoc.array();
            
            // 处理结果
            QMetaObject::invokeMethod(dlg, [dlg, json_group_list]() {
                if (!dlg) return;
                
                auto normalizeGroupType = [](const QJsonValue& v) -> QString {
                    if (v.isString()) {
                        return v.toString().trimmed();
                    }
                    if (v.isDouble()) {
                        const int t = v.toInt();
                        switch (t) {
                        case kTIMGroup_Public:    return QStringLiteral("Public");
                        case kTIMGroup_Private:   return QStringLiteral("Private");
                        case kTIMGroup_ChatRoom:  return QStringLiteral("ChatRoom");
                        case kTIMGroup_BChatRoom: return QStringLiteral("BChatRoom");
                        case kTIMGroup_AVChatRoom:return QStringLiteral("AVChatRoom");
                        default:                  return QString::number(t);
                        }
                    }
                    return QString();
                };
                
                int publicGroupCount = 0;
                for (int i = 0; i < json_group_list.size(); i++) {
                    QJsonObject group = json_group_list[i].toObject();
                    
                    // 获取群组ID
                    QString groupId = group[kTIMGroupBaseInfoGroupId].toString();
                    if (groupId.isEmpty()) {
                        continue;
                    }
                    
                    // 获取群类型
                    const QString groupType = normalizeGroupType(group.value(kTIMGroupBaseInfoGroupType));
                    
                    // 只处理Public类型的普通群
                    if (groupType == "Public") {
                        dlg->m_joinedGroupIds.insert(groupId);
                        publicGroupCount++;
                    }
                }
                
                qDebug() << "从腾讯IM加载已加入普通群列表，共" << publicGroupCount << "个普通群";
            }, Qt::QueuedConnection);
        }, this);
        
        if (ret != TIM_SUCC) {
            qDebug() << "调用TIMGroupGetJoinedGroupList失败，错误码:" << ret;
        }
    }
    
    // 从服务器接口获取已加入的班级群列表
    void loadJoinedClassGroupsFromServer()
    {
        UserInfo userInfo = CommonInfo::GetData();
        if (!userInfo.teacher_unique_id.isEmpty())
        {
            QUrl url("http://47.100.126.194:5000/groups/by-teacher");
            QUrlQuery query;
            query.addQueryItem("teacher_unique_id", userInfo.teacher_unique_id);
            url.setQuery(query);
            
            // 使用QNetworkAccessManager来避免与TAHttpHandler冲突
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
                            
                            int classGroupCount = 0;
                            
                            // 处理群主群组列表（班级群）
                            if (dataObj["owner_groups"].isArray()) {
                                QJsonArray ownerGroups = dataObj["owner_groups"].toArray();
                                for (int i = 0; i < ownerGroups.size(); i++) {
                                    QJsonObject group = ownerGroups[i].toObject();
                                    QString groupId = group["group_id"].toString();
                                    if (!groupId.isEmpty()) {
                                        m_joinedGroupIds.insert(groupId);
                                        classGroupCount++;
                                    }
                                }
                            }
                            
                            // 处理成员群组列表（班级群）
                            if (dataObj["member_groups"].isArray()) {
                                QJsonArray memberGroups = dataObj["member_groups"].toArray();
                                for (int i = 0; i < memberGroups.size(); i++) {
                                    QJsonObject group = memberGroups[i].toObject();
                                    QString groupId = group["group_id"].toString();
                                    if (!groupId.isEmpty()) {
                                        m_joinedGroupIds.insert(groupId);
                                        classGroupCount++;
                                    }
                                }
                            }
                            
                            qDebug() << "从服务器加载已加入班级群列表，共" << classGroupCount << "个班级群";
                            qDebug() << "已加入群组总数（普通群+班级群）:" << m_joinedGroupIds.size();
                        }
                    }
                } else {
                    qDebug() << "加载已加入班级群列表失败:" << reply->errorString();
                }
                reply->deleteLater();
                tempManager->deleteLater();
            });
        }
    }
    
    void loadFriends()
    {
        // 通过/friends接口获取当前用户的好友列表
        UserInfo userInfo = CommonInfo::GetData();
        if (m_httpHandler && !userInfo.strIdNumber.isEmpty())
        {
            QUrl url("http://47.100.126.194:5000/friends");
            QUrlQuery query;
            query.addQueryItem("id_card", userInfo.strIdNumber);
            url.setQuery(query);
            
            // 使用QNetworkAccessManager来避免与TAHttpHandler冲突
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
                        if (obj.contains("friends") && obj["friends"].isArray()) {
                            QJsonArray friendsArray = obj["friends"].toArray();
                            m_friendIds.clear();
                            
                            for (int i = 0; i < friendsArray.size(); i++) {
                                QJsonObject friendObj = friendsArray[i].toObject();
                                if (friendObj.contains("teacher_info") && friendObj["teacher_info"].isObject()) {
                                    QJsonObject teacherInfo = friendObj["teacher_info"].toObject();
                                    QString teacherUniqueId = teacherInfo["teacher_unique_id"].toString();
                                    if (!teacherUniqueId.isEmpty()) {
                                        m_friendIds.insert(teacherUniqueId);
                                    }
                                }
                            }
                            
                            qDebug() << "已加载好友列表，共" << m_friendIds.size() << "个好友";
                        }
                    }
                } else {
                    qDebug() << "加载好友列表失败:" << reply->errorString();
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
        QString userId = userInfo.teacher_unique_id;
        QString userName = userInfo.strName;
        
        if (userId.isEmpty() || userName.isEmpty()) {
            QMessageBox::warning(this, "错误", "用户信息不完整，无法加入群组！");
            return;
        }
        
        // 从搜索结果中查找这个群，判断是班级群还是普通群
        bool isClassGroup = false;
        for (const QJsonObject& item : m_searchGroupResults) {
            if (item["group_id"].toString() == groupId) {
                QString classid = item["classid"].toString();
                isClassGroup = !classid.isEmpty(); // classid不为空表示是班级群
                break;
            }
        }
        
        // 弹出输入框让用户输入加入理由
        bool ok;
        QString reason = SearchInputDialog::getText(this, "申请加入群组", 
            QString("请输入加入群组 \"%1\" 的理由：").arg(groupName),
            "", &ok);
        
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
            bool isClassGroup; // 是否是班级群
        };
        
        JoinGroupCallbackData* callbackData = new JoinGroupCallbackData;
        callbackData->dlg = this;
        callbackData->groupId = groupId;
        callbackData->groupName = groupName;
        callbackData->userId = userId;
        callbackData->userName = userName;
        callbackData->reason = reason;
        callbackData->isClassGroup = isClassGroup;
        
        // 调用腾讯SDK的申请加入群组接口
        QByteArray groupIdBytes = groupId.toUtf8();
        QByteArray reasonBytes = reason.toUtf8();
        
        int ret = TIMGroupJoin(groupIdBytes.constData(), reasonBytes.constData(),
            [](int32_t code, const char* desc, const char* json_params, const void* user_data) {
                JoinGroupCallbackData* data = (JoinGroupCallbackData*)user_data;
                SearchDialog* dlg = data->dlg;
                
                if (code == TIM_SUCC) {
                    qDebug() << "腾讯SDK申请加入群组成功:" << data->groupId << "是否班级群:" << data->isClassGroup;
                    
                    // 如果是班级群，需要调用服务器接口
                    if (data->isClassGroup) {
                        dlg->sendJoinGroupRequestToServer(data->groupId, data->userId, data->userName, data->reason);
                    } else {
                        // 普通群：只使用腾讯IM接口，成功后刷新已加入群组列表
                        dlg->m_joinedGroupIds.insert(data->groupId);
                        // 发出加入群组成功信号
                        emit dlg->groupJoined(data->groupId);
                    }
                    
                    // 显示成功消息
                    SearchMessageDialog* msgDlg = new SearchMessageDialog(dlg);
                    msgDlg->setTitle("申请成功");
                    msgDlg->setMessage(QString("已成功申请加入群组 \"%1\"！\n等待管理员审核。").arg(data->groupName));
                    msgDlg->exec();
                } else {
                    QString errorDesc = QString::fromUtf8(desc ? desc : "未知错误");
                    QString errorMsg = QString("申请加入群组失败\n错误码: %1\n错误描述: %2").arg(code).arg(errorDesc);
                    qDebug() << errorMsg;
                    SearchMessageDialog* msgDlg = new SearchMessageDialog(dlg);
                    msgDlg->setTitle("申请失败");
                    msgDlg->setMessage(errorMsg);
                    msgDlg->exec();
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
        const QString& tag, const QString& desc, const QString& id = "", bool isMember = false, const QString& type = "group")
    {
        QHBoxLayout* itemLayout = new QHBoxLayout;
        QLabel* avatar = new QLabel;
        avatar->setFixedSize(40, 40);
        avatar->setStyleSheet("background-color: #565656; border-radius: 20px;"); // 用灰色代替头像，可以换成 QPixmap 加载图片

        QVBoxLayout* infoLayout = new QVBoxLayout;
        QLabel* lblName = new QLabel(QString("%1  ⛔ %2人  %3").arg(name).arg(memberCount).arg(tag));
        lblName->setStyleSheet("background-color: #565656; color: #f5f5f5; font-weight: bold;");
        QLabel* lblDesc = new QLabel(desc);
        lblDesc->setStyleSheet("background-color: #565656; color: #d3d3d3; font-size: 12px;");
        infoLayout->addWidget(lblName);
        infoLayout->addWidget(lblDesc);

        itemLayout->addWidget(avatar);
        itemLayout->addLayout(infoLayout);
        itemLayout->addStretch();
        
        // 根据类型显示不同的按钮
        if (type == "class") {
            // 班级：显示"加入"按钮
            QPushButton* btnJoin = new QPushButton("加入");
            btnJoin->setStyleSheet("background-color: #3569ff; color: #ffffff; padding: 4px 12px; border-radius: 15px;");
            connect(btnJoin, &QPushButton::clicked, this, [=]() {
                onJoinClassClicked(id);
            });
            itemLayout->addWidget(btnJoin);
        } else if (type == "teacher") {
            // 教师：显示"添加好友"或"已添加"按钮
            QPushButton* btnAddFriend = new QPushButton(isMember ? "已添加" : "添加好友");
            
            if (isMember) {
                // 已经是好友，按钮置灰
                btnAddFriend->setEnabled(false);
                btnAddFriend->setStyleSheet("background-color: #454545; color: #d3d3d3; padding: 4px 12px; border-radius: 15px;");
            } else {
                // 还不是好友，可以点击添加
                btnAddFriend->setEnabled(true);
                btnAddFriend->setStyleSheet("background-color: #3569ff; color: #ffffff; padding: 4px 12px; border-radius: 15px;");
                connect(btnAddFriend, &QPushButton::clicked, this, [=]() {
                    onAddFriendClicked(id);
                });
                // 保存按钮引用，以便后续更新状态
                m_addFriendButtons[id] = btnAddFriend;
            }
            itemLayout->addWidget(btnAddFriend);
        } else if (type == "group" && !id.isEmpty()) {
            // 群组：显示"加入"或"已加入"按钮
            QPushButton* btnJoin = new QPushButton(isMember ? "已加入" : "加入");
            
            if (isMember) {
                btnJoin->setEnabled(false);
                btnJoin->setStyleSheet("background-color: #454545; color: #d3d3d3; padding: 4px 8px; border-radius: 15px;");
            } else {
                btnJoin->setEnabled(true);
                btnJoin->setStyleSheet("background-color: #454545; color: #f5f5f5; padding: 4px 8px; border-radius: 15px;");
                connect(btnJoin, &QPushButton::clicked, this, [=]() {
                    onJoinGroupClicked(id, name);
                });
            }
            
            btnJoin->setProperty("group_id", id);
            btnJoin->setProperty("group_name", name);
            itemLayout->addWidget(btnJoin);
        }

        QFrame* frame = new QFrame;
        frame->setLayout(itemLayout);
        frame->setFrameShape(QFrame::HLine);
        frame->setStyleSheet("QFrame { background-color: #565656; }");
        parent->insertWidget(parent->count() - 1, frame); // 在stretch之前插入
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragStartPos);
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = false;
            event->accept();
        }
    }

    void enterEvent(QEvent* event) override
    {
        if (m_closeButton) {
            m_closeButton->show();
        }
        QDialog::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        if (m_closeButton) {
            m_closeButton->hide();
        }
        QDialog::leaveEvent(event);
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QDialog::resizeEvent(event);
        // 更新关闭按钮位置
        if (m_closeButton) {
            m_closeButton->move(width() - 35, 5);
        }
    }

private:
    TAHttpHandler* m_httpHandler = nullptr;
    TaQTWebSocket* m_pWs = nullptr; // WebSocket 实例
    QLineEdit* m_editSearch = nullptr;
    QLabel* m_lblNum = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_listContainer = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QString m_currentUserId; // 当前用户ID
    QSet<QString> m_joinedGroupIds; // 已加入的群组ID集合
    QSet<QString> m_friendIds; // 已添加的好友ID集合（teacher_unique_id）
    QMap<QString, QPushButton*> m_addFriendButtons; // 保存"添加好友"按钮：key为teacher_unique_id
    int m_groupCount = 0; // 群组搜索结果数量
    int m_teacherCount = 0; // 教师搜索结果数量
    int m_classCount = 0; // 班级搜索结果数量
    bool m_dragging = false; // 是否正在拖动
    QPoint m_dragStartPos; // 拖动起始位置
    QPushButton* m_closeButton = nullptr; // 关闭按钮
    
    // 分类按钮
    QPushButton* m_btnAll = nullptr;
    QPushButton* m_btnClass = nullptr;
    QPushButton* m_btnTeacher = nullptr;
    QPushButton* m_btnGroup = nullptr;
    QButtonGroup* m_categoryButtonGroup = nullptr;
    int m_currentSearchCategory = 0; // 0:全部, 1:班级, 2:教师, 3:群
    
    // 搜索结果存储
    QList<QJsonObject> m_searchClassResults;
    QList<QJsonObject> m_searchTeacherResults;
    QList<QJsonObject> m_searchGroupResults;
    int m_pendingSearchRequests = 0; // 等待的搜索请求数量
};

