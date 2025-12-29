#include "WallpaperDialog.h"
#include <QDebug>
#include <QUrlQuery>
#include <QDateTime>
#include <QCoreApplication>
#include <QEvent>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDrag>

WallpaperDialog::WallpaperDialog(QWidget* parent)
    : QDialog(parent)
{
    // 移除标题栏
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    
    setWindowTitle("班级壁纸");
    resize(900, 700);
    setStyleSheet("background-color:#2b2b2b; color: white;");
    
    m_httpHandler = new TAHttpHandler(this);
    m_networkManager = new QNetworkAccessManager(this);
    
    setupUI();
    
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
}

WallpaperDialog::~WallpaperDialog()
{
}

void WallpaperDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // 顶部栏：班级壁纸按钮（左上角）和关闭按钮（右上角）
    QHBoxLayout* topLayout = new QHBoxLayout;
    
    // 班级壁纸按钮（左上角）
    m_classWallpaperBtn = new QPushButton("班级壁纸", this);
    QString buttonStyle = 
        "QPushButton {"
        "background-color: #555;"
        "color: white;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #666;"
        "}"
        "QPushButton:pressed {"
        "background-color: #444;"
        "}";
    
    QString activeButtonStyle = 
        "QPushButton {"
        "background-color: #608bd0;"
        "color: white;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #4f78be;"
        "}";
    
    m_classWallpaperBtn->setStyleSheet(buttonStyle);
    connect(m_classWallpaperBtn, &QPushButton::clicked, this, &WallpaperDialog::onClassWallpaperClicked);
    topLayout->addWidget(m_classWallpaperBtn);
    
    // 添加stretch，将右侧按钮推到右边
    topLayout->addStretch();
    
    // 其他功能按钮：一周壁纸、设为壁纸、壁纸库（在右侧，关闭按钮左边）
    m_weeklyWallpaperBtn = new QPushButton("一周壁纸", this);
    m_setWallpaperBtn = new QPushButton("设为壁纸", this);
    m_wallpaperLibraryBtn = new QPushButton("壁纸库", this);
    
    m_weeklyWallpaperBtn->setStyleSheet(buttonStyle);
    m_setWallpaperBtn->setStyleSheet(buttonStyle);
    m_wallpaperLibraryBtn->setStyleSheet(buttonStyle);
    
    connect(m_weeklyWallpaperBtn, &QPushButton::clicked, this, &WallpaperDialog::onWeeklyWallpaperClicked);
    connect(m_setWallpaperBtn, &QPushButton::clicked, this, &WallpaperDialog::onSetWallpaperClicked);
    connect(m_wallpaperLibraryBtn, &QPushButton::clicked, this, &WallpaperDialog::onWallpaperLibraryClicked);
    
    topLayout->addWidget(m_weeklyWallpaperBtn);
    topLayout->addWidget(m_setWallpaperBtn);
    topLayout->addWidget(m_wallpaperLibraryBtn);
    
    // 关闭按钮（右上角）
    m_closeButton = new QPushButton("✕", this);
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "background-color: transparent;"
        "color: white;"
        "font-size: 18px;"
        "font-weight: bold;"
        "border: none;"
        "}"
        "QPushButton:hover {"
        "background-color: #444;"
        "border-radius: 15px;"
        "}"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &WallpaperDialog::onCloseClicked);
    topLayout->addWidget(m_closeButton);
    
    m_mainLayout->addLayout(topLayout);
    
    // 一周壁纸设置区（上栏，默认隐藏）
    setupWeeklyWallpaperSection();
    m_weeklyWallpaperWidget->hide();
    
    // 内容区域：滚动区域（下栏）
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #2b2b2b; }");
    
    m_contentWidget = new QWidget(this);
    m_gridLayout = new QGridLayout(m_contentWidget);
    m_gridLayout->setSpacing(15);
    m_gridLayout->setContentsMargins(10, 10, 10, 10);
    
    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea, 1);
    
    // 初始显示班级壁纸页面
    showClassWallpaperPage();
    
    // 加载一周壁纸配置
    loadWeeklyWallpaperConfig();
}

void WallpaperDialog::showClassWallpaperPage()
{
    m_currentPage = ClassWallpaperPage;
    
    // 更新按钮样式
    m_classWallpaperBtn->setStyleSheet(
        "QPushButton {"
        "background-color: #608bd0;"
        "color: white;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
    );
    m_weeklyWallpaperBtn->setStyleSheet(
        "QPushButton {"
        "background-color: #555;"
        "color: white;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #666;"
        "}"
    );
    m_setWallpaperBtn->setStyleSheet(
        "QPushButton {"
        "background-color: #555;"
        "color: white;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #666;"
        "}"
    );
    m_wallpaperLibraryBtn->setStyleSheet(
        "QPushButton {"
        "background-color: #555;"
        "color: white;"
        "padding: 8px 16px;"
        "border-radius: 4px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #666;"
        "}"
    );
    
    // 隐藏返回、下载按钮和壁纸库标签，显示所有顶部按钮
    if (m_backButton) {
        m_backButton->hide();
    }
    if (m_downloadButton) {
        m_downloadButton->hide();
    }
    if (m_wallpaperLibraryLabel) {
        m_wallpaperLibraryLabel->hide();
    }
    
    // 显示所有顶部按钮
    m_classWallpaperBtn->show();
    m_weeklyWallpaperBtn->show();
    m_setWallpaperBtn->show();
    m_wallpaperLibraryBtn->show();
    m_closeButton->show();
    
    // 加载班级壁纸
    loadClassWallpapers();
}

void WallpaperDialog::showWallpaperLibraryPage()
{
    m_currentPage = WallpaperLibraryPage;
    
    // 隐藏所有顶部按钮
    m_classWallpaperBtn->hide();
    m_weeklyWallpaperBtn->hide();
    m_setWallpaperBtn->hide();
    m_wallpaperLibraryBtn->hide();
    m_closeButton->hide();
    
    // 隐藏一周壁纸设置区
    if (m_weeklyWallpaperWidget) {
        m_weeklyWallpaperWidget->hide();
    }
    m_weeklyWallpaperVisible = false;  // 重置一周壁纸设置区可见状态
    
    // 获取顶部布局
    QHBoxLayout* topLayout = qobject_cast<QHBoxLayout*>(m_mainLayout->itemAt(0)->layout());
    if (!topLayout) {
        return;
    }
    
    // 创建返回按钮（如果不存在）- 在壁纸库标签左边
    if (!m_backButton) {
        m_backButton = new QPushButton("←", this);
        m_backButton->setFixedSize(40, 30);
        m_backButton->setStyleSheet(
            "QPushButton {"
            "background-color: #555;"
            "color: white;"
            "font-size: 18px;"
            "font-weight: bold;"
            "border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "background-color: #666;"
            "}"
        );
        connect(m_backButton, &QPushButton::clicked, this, &WallpaperDialog::onBackClicked);
        topLayout->insertWidget(0, m_backButton);
    }
    m_backButton->show();
    
    // 创建"壁纸库"标签（如果不存在）- 在返回按钮右边
    if (!m_wallpaperLibraryLabel) {
        m_wallpaperLibraryLabel = new QLabel("壁纸库", this);
        m_wallpaperLibraryLabel->setStyleSheet(
            "QLabel {"
            "color: white;"
            "font-size: 18px;"
            "font-weight: bold;"
            "padding: 5px;"
            "}"
        );
        topLayout->insertWidget(1, m_wallpaperLibraryLabel);
    }
    m_wallpaperLibraryLabel->show();
    
    // 创建下载按钮（如果不存在）
    if (!m_downloadButton) {
        m_downloadButton = new QPushButton("下载", this);
        m_downloadButton->setFixedSize(80, 30);
        m_downloadButton->setStyleSheet(
            "QPushButton {"
            "background-color: #608bd0;"
            "color: white;"
            "padding: 8px 16px;"
            "border-radius: 4px;"
            "font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "background-color: #4f78be;"
            "}"
        );
        connect(m_downloadButton, &QPushButton::clicked, this, &WallpaperDialog::onDownloadClicked);
        // 添加到布局末尾（右上角）
        topLayout->addWidget(m_downloadButton);
    } else {
        // 如果按钮已存在但不在布局中，添加到布局末尾
        if (m_downloadButton->parent() != this || topLayout->indexOf(m_downloadButton) == -1) {
            topLayout->addWidget(m_downloadButton);
        }
    }
    m_downloadButton->show();
    
    // 加载壁纸库
    loadWallpaperLibrary();
}

void WallpaperDialog::loadClassWallpapers()
{
    // 清空当前内容
    clearLayout(m_gridLayout);
    
    // 从服务器获取班级壁纸列表
    if (m_httpHandler && !m_groupId.isEmpty()) {
        QUrl url("http://47.100.126.194:5000/class-wallpapers");
        QUrlQuery query;
        query.addQueryItem("group_id", m_groupId);
        url.setQuery(query);
        m_httpHandler->get(url.toString());
    }
    
    // 添加"添加本地壁纸"按钮
    QWidget* addWidget = new QWidget(m_contentWidget);
    addWidget->setFixedSize(150, 150);
    addWidget->setStyleSheet(
        "QWidget {"
        "background-color: #444;"
        "border: 2px dashed #888;"
        "border-radius: 8px;"
        "}"
    );
    
    QVBoxLayout* addLayout = new QVBoxLayout(addWidget);
    addLayout->setAlignment(Qt::AlignCenter);
    
    QLabel* plusLabel = new QLabel("+", addWidget);
    plusLabel->setAlignment(Qt::AlignCenter);
    plusLabel->setStyleSheet("color: white; font-size: 48px; font-weight: bold;");
    
    QLabel* textLabel = new QLabel("添加本地壁纸", addWidget);
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setStyleSheet("color: #aaa; font-size: 12px;");
    
    addLayout->addWidget(plusLabel);
    addLayout->addWidget(textLabel);
    
    // 使整个widget可点击
    addWidget->installEventFilter(this);
    addWidget->setProperty("isAddButton", true);
    
    m_gridLayout->addWidget(addWidget, 0, 0);
}

void WallpaperDialog::loadWallpaperLibrary()
{
    // 清空当前内容
    clearLayout(m_gridLayout);
    
    // 从服务器获取壁纸库列表
    if (m_httpHandler) {
        QUrl url("http://47.100.126.194:5000/wallpaper-library");
        m_httpHandler->get(url.toString());
    }
}

void WallpaperDialog::onCloseClicked()
{
    close();
}

void WallpaperDialog::onWeeklyWallpaperClicked()
{
    // 切换一周壁纸设置区的显示/隐藏
    toggleWeeklyWallpaperSection();
}

void WallpaperDialog::onSetWallpaperClicked()
{
    if (m_selectedWallpaperId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择一张壁纸");
        return;
    }
    
    // 设为壁纸会停用一周壁纸功能
    // 先停用一周壁纸（如果有启用的话）
    if (m_weeklyWallpaperEnabled) {
        disableWeeklyWallpaper();
        // 注意：setWallpaperToClass 会在停用成功后调用
        // 这里先返回，等待停用接口返回后再设置壁纸
        return;
    }
    
    setWallpaperToClass(m_selectedWallpaperId);
}

void WallpaperDialog::onClassWallpaperClicked()
{
    showClassWallpaperPage();
}

void WallpaperDialog::onWallpaperLibraryClicked()
{
    showWallpaperLibraryPage();
}

void WallpaperDialog::onBackClicked()
{
    showClassWallpaperPage();
}

void WallpaperDialog::onDownloadClicked()
{
    if (m_selectedLibraryWallpaperId <= 0) {
        QMessageBox::warning(this, "提示", "请先选择一张壁纸");
        return;
    }
    
    // 从壁纸库下载壁纸到班级壁纸
    downloadWallpaperFromLibrary(m_selectedLibraryWallpaperId);
}

void WallpaperDialog::onAddLocalWallpaperClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择壁纸图片",
        "",
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
    );
    
    if (filePath.isEmpty()) {
        return;
    }
    
    // 上传到服务器并保存到班级壁纸
    if (m_httpHandler && !m_groupId.isEmpty()) {
        QJsonObject payload;
        payload["group_id"] = m_groupId;
        payload["file_path"] = filePath;
        
        QJsonDocument doc(payload);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QUrl url("http://47.100.126.194:5000/groups/wallpapers/upload");
        m_httpHandler->post(url.toString(), jsonData);
    }
}

void WallpaperDialog::onWallpaperSelected(const QString& wallpaperPath)
{
    m_selectedWallpaperPath = wallpaperPath;
    qDebug() << "选中壁纸:" << wallpaperPath;
}

void WallpaperDialog::onHttpSuccess(const QString& resp)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "JSON解析失败";
        return;
    }
    
    // 根据接口文档，响应格式是 { "data": { ... } }
    QJsonObject rootObj = doc.object();
    QJsonObject dataObj = rootObj.value("data").toObject();
    int code = dataObj.value("code").toInt(-1);
    
    if (code != 200) {
        QString message = dataObj.value("message").toString("请求失败");
        QMessageBox::warning(this, "提示", message);
        return;
    }
    
    if (m_currentPage == ClassWallpaperPage) {
        // 处理班级壁纸列表
        QJsonArray wallpapers = dataObj.value("wallpapers").toArray();
        
        int row = 0, col = 0;
        const int colsPerRow = 4;
        
        for (int i = 0; i < wallpapers.size(); ++i) {
            QJsonObject wp = wallpapers[i].toObject();
            int wallpaperId = wp.value("id").toInt(-1);
            QString imageUrl = wp.value("image_url").toString();
            QString name = wp.value("name").toString();
            int isCurrent = wp.value("is_current").toInt(0);
            
            QString label = name;
            if (label.isEmpty()) {
                label = QString("图%1").arg(i + 1);
            }
            if (isCurrent == 1) {
                label += " (当前)";
            }
            
            QWidget* thumbnail = createWallpaperThumbnail(imageUrl, label, true);
            thumbnail->setProperty("wallpaperId", wallpaperId);  // 保存班级壁纸ID
            thumbnail->setProperty("wallpaperPath", imageUrl);
            m_gridLayout->addWidget(thumbnail, row, col);
            
            col++;
            if (col >= colsPerRow) {
                col = 0;
                row++;
            }
        }
        
        // 添加"添加本地壁纸"按钮
        QWidget* addWidget = new QWidget(m_contentWidget);
        addWidget->setFixedSize(150, 150);
        addWidget->setStyleSheet(
            "QWidget {"
            "background-color: #444;"
            "border: 2px dashed #888;"
            "border-radius: 8px;"
            "}"
        );
        
        QVBoxLayout* addLayout = new QVBoxLayout(addWidget);
        addLayout->setAlignment(Qt::AlignCenter);
        
        QLabel* plusLabel = new QLabel("+", addWidget);
        plusLabel->setAlignment(Qt::AlignCenter);
        plusLabel->setStyleSheet("color: white; font-size: 48px; font-weight: bold;");
        
        QLabel* textLabel = new QLabel("添加本地壁纸", addWidget);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setStyleSheet("color: #aaa; font-size: 12px;");
        
        addLayout->addWidget(plusLabel);
        addLayout->addWidget(textLabel);
        
        addWidget->installEventFilter(this);
        addWidget->setProperty("isAddButton", true);
        
        m_gridLayout->addWidget(addWidget, row, col);
    } else if (m_currentPage == WallpaperLibraryPage) {
        // 处理壁纸库列表
        QJsonArray wallpapers = dataObj.value("wallpapers").toArray();
        
        int row = 0, col = 0;
        const int colsPerRow = 4;
        
        for (int i = 0; i < wallpapers.size(); ++i) {
            QJsonObject wp = wallpapers[i].toObject();
            int wallpaperId = wp.value("id").toInt(-1);
            QString imageUrl = wp.value("image_url").toString();
            QString name = wp.value("name").toString();
            
            QString label = name;
            if (label.isEmpty()) {
                label = QString("图%1").arg(i + 1);
            }
            
            QWidget* thumbnail = createWallpaperThumbnail(imageUrl, label, true);
            thumbnail->setProperty("libraryWallpaperId", wallpaperId);  // 保存壁纸库ID
            thumbnail->setProperty("wallpaperPath", imageUrl);
            m_gridLayout->addWidget(thumbnail, row, col);
            
            col++;
            if (col >= colsPerRow) {
                col = 0;
                row++;
            }
        }
    }
}

void WallpaperDialog::onHttpFailed(const QString& err)
{
    QMessageBox::critical(this, "错误", "请求失败：" + err);
}

void WallpaperDialog::onImageDownloaded(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "错误", "下载失败：" + reply->errorString());
        reply->deleteLater();
        return;
    }
    
    // 保存图片到本地
    QByteArray imageData = reply->readAll();
    QString filename = reply->url().fileName();
    if (filename.isEmpty()) {
        filename = "wallpaper_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png";
    }
    
    QString saveDir = QCoreApplication::applicationDirPath() + "/wallpapers/" + m_groupId;
    QDir dir;
    if (!dir.exists(saveDir)) {
        dir.mkpath(saveDir);
    }
    
    QString filePath = saveDir + "/" + filename;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();
        
        // 上传到服务器
        if (m_httpHandler && !m_groupId.isEmpty()) {
            QJsonObject payload;
            payload["group_id"] = m_groupId;
            payload["file_path"] = filePath;
            
            QJsonDocument doc(payload);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            
            QUrl url("http://47.100.126.194:5000/groups/wallpapers/upload");
            m_httpHandler->post(url.toString(), jsonData);
        }
        
        QMessageBox::information(this, "提示", "壁纸下载成功");
        loadClassWallpapers(); // 刷新班级壁纸列表
    } else {
        QMessageBox::critical(this, "错误", "保存文件失败");
    }
    
    reply->deleteLater();
}

void WallpaperDialog::onDownloadWallpaperSuccess(const QString& resp)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onDownloadWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onDownloadWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject rootObj = doc.object();
        QJsonObject dataObj = rootObj.value("data").toObject();
        int code = dataObj.value("code").toInt(-1);
        QString message = dataObj.value("message").toString("下载成功");
        
        if (code == 200) {
            QMessageBox::information(this, "提示", message);
            // 刷新班级壁纸列表
            loadClassWallpapers();
        } else {
            QMessageBox::warning(this, "提示", message);
        }
    } else {
        QMessageBox::information(this, "提示", "壁纸下载成功");
        loadClassWallpapers();
    }
}

void WallpaperDialog::onDownloadWallpaperFailed(const QString& err)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onDownloadWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onDownloadWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QMessageBox::critical(this, "错误", "下载壁纸失败：" + err);
}

void WallpaperDialog::onSetWallpaperSuccess(const QString& resp)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onSetWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onSetWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject rootObj = doc.object();
        QJsonObject dataObj = rootObj.value("data").toObject();
        int code = dataObj.value("code").toInt(-1);
        QString message = dataObj.value("message").toString("设置成功");
        
        if (code == 200) {
            QMessageBox::information(this, "提示", message);
            // 刷新班级壁纸列表
            loadClassWallpapers();
        } else {
            QMessageBox::warning(this, "提示", message);
        }
    } else {
        QMessageBox::information(this, "提示", "壁纸设置成功");
        loadClassWallpapers();
    }
}

void WallpaperDialog::onSetWallpaperFailed(const QString& err)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onSetWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onSetWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QMessageBox::critical(this, "错误", "设置壁纸失败：" + err);
}

void WallpaperDialog::downloadWallpaperFromLibrary(int libraryWallpaperId)
{
    if (m_httpHandler && !m_groupId.isEmpty() && libraryWallpaperId > 0) {
        // 临时断开原有的信号连接
        disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
        disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
        
        // 连接下载壁纸专用的信号
        connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onDownloadWallpaperSuccess, Qt::UniqueConnection);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onDownloadWallpaperFailed, Qt::UniqueConnection);
        
        QJsonObject payload;
        payload["group_id"] = m_groupId;
        payload["wallpaper_id"] = libraryWallpaperId;  // 壁纸库的ID
        
        QJsonDocument doc(payload);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QUrl url("http://47.100.126.194:5000/class-wallpapers/download");
        m_httpHandler->post(url.toString(), jsonData);
    }
}

void WallpaperDialog::setWallpaperToClass(int wallpaperId)
{
    if (m_httpHandler && !m_groupId.isEmpty() && wallpaperId > 0) {
        // 临时断开原有的信号连接
        disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
        disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
        
        // 连接设置壁纸专用的信号
        connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onSetWallpaperSuccess, Qt::UniqueConnection);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onSetWallpaperFailed, Qt::UniqueConnection);
        
        QJsonObject payload;
        payload["group_id"] = m_groupId;
        payload["wallpaper_id"] = wallpaperId;  // 班级壁纸的ID
        
        QJsonDocument doc(payload);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QUrl url("http://47.100.126.194:5000/class-wallpapers/set-current");
        m_httpHandler->post(url.toString(), jsonData);
    }
}

QWidget* WallpaperDialog::createWallpaperThumbnail(const QString& imagePath, const QString& label, bool isSelectable)
{
    QWidget* container = new QWidget(m_contentWidget);
    container->setFixedSize(150, 180);
    
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    
    // 图片标签
    QLabel* imageLabel = new QLabel(container);
    imageLabel->setFixedSize(150, 150);
    imageLabel->setScaledContents(true);
    imageLabel->setStyleSheet(
        "QLabel {"
        "background-color: #444;"
        "border: 2px solid #666;"
        "border-radius: 8px;"
        "}"
    );
    
    // 加载图片
    if (imagePath.startsWith("http://") || imagePath.startsWith("https://")) {
        // 网络图片，异步加载
        QUrl imageUrl(imagePath);
        QNetworkRequest request(imageUrl);
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, [imageLabel, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                if (!pixmap.isNull()) {
                    imageLabel->setPixmap(pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
            reply->deleteLater();
        });
    } else {
        // 本地图片
        QPixmap pixmap(imagePath);
        if (!pixmap.isNull()) {
            imageLabel->setPixmap(pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    
    // 标签
    QLabel* labelWidget = new QLabel(label, container);
    labelWidget->setAlignment(Qt::AlignCenter);
    labelWidget->setStyleSheet("color: white; font-size: 12px;");
    
    layout->addWidget(imageLabel);
    layout->addWidget(labelWidget);
    
    // 如果可选中，添加点击事件和拖拽功能
    if (isSelectable) {
        container->installEventFilter(this);
        container->setProperty("wallpaperPath", imagePath);
        container->setProperty("isSelectable", true);
        
        // 启用拖拽
        container->setAcceptDrops(false);
        imageLabel->setAcceptDrops(false);
        container->setProperty("draggable", true);
    }
    
    return container;
}

void WallpaperDialog::clearLayout(QLayout* layout)
{
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        } else if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}

void WallpaperDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (child == m_closeButton) {
            QDialog::mousePressEvent(event);
            return;
        }
        
        // 检查是否点击了壁纸缩略图
        if (child && child->property("isSelectable").toBool()) {
            QString wallpaperPath = child->property("wallpaperPath").toString();
            if (!wallpaperPath.isEmpty()) {
                onWallpaperSelected(wallpaperPath);
                // 更新选中状态（可以添加视觉反馈）
                return;
            }
        }
        
        // 检查是否点击了添加按钮
        if (child && child->property("isAddButton").toBool()) {
            onAddLocalWallpaperClicked();
            return;
        }
        
        m_dragging = true;
        m_dragStartPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QDialog::mousePressEvent(event);
}

void WallpaperDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragStartPosition);
        event->accept();
    }
    QDialog::mouseMoveEvent(event);
}

void WallpaperDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
    QDialog::mouseReleaseEvent(event);
}

bool WallpaperDialog::eventFilter(QObject* obj, QEvent* event)
{
    // 处理一周壁纸槽位的拖放事件
    if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent* dragEvent = static_cast<QDragEnterEvent*>(event);
        QLabel* slotLabel = qobject_cast<QLabel*>(obj);
        if (slotLabel && slotLabel->property("isWeeklySlot").toBool()) {
            if (dragEvent->mimeData()->hasText() || dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        }
    } else if (event->type() == QEvent::Drop) {
        QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
        QLabel* slotLabel = qobject_cast<QLabel*>(obj);
        if (slotLabel && slotLabel->property("isWeeklySlot").toBool()) {
            int dayIndex = slotLabel->property("dayIndex").toInt();
            
            // 获取拖放的壁纸路径
            QString wallpaperPath;
            if (dropEvent->mimeData()->hasText()) {
                wallpaperPath = dropEvent->mimeData()->text();
            } else if (dropEvent->mimeData()->hasUrls() && !dropEvent->mimeData()->urls().isEmpty()) {
                QUrl url = dropEvent->mimeData()->urls().first();
                if (url.isLocalFile()) {
                    wallpaperPath = url.toLocalFile();
                } else {
                    wallpaperPath = url.toString();
                }
            }
            
            if (!wallpaperPath.isEmpty()) {
                // 保存壁纸路径
                m_weeklyWallpapers[dayIndex] = wallpaperPath;
                
                // 更新标签显示
                updateWeeklyWallpaperSlot(dayIndex, wallpaperPath);
                
                dropEvent->acceptProposedAction();
                return true;
            }
        }
    }
    
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QWidget* widget = qobject_cast<QWidget*>(obj);
            if (widget) {
                if (widget->property("isSelectable").toBool()) {
                    QString wallpaperPath = widget->property("wallpaperPath").toString();
                    int wallpaperId = widget->property("wallpaperId").toInt();
                    int libraryWallpaperId = widget->property("libraryWallpaperId").toInt();
                    
                    if (!wallpaperPath.isEmpty()) {
                        m_selectedWallpaperPath = wallpaperPath;
                        if (wallpaperId > 0) {
                            // 班级壁纸页面
                            m_selectedWallpaperId = wallpaperId;
                            m_selectedLibraryWallpaperId = -1;
                        } else if (libraryWallpaperId > 0) {
                            // 壁纸库页面
                            m_selectedLibraryWallpaperId = libraryWallpaperId;
                            m_selectedWallpaperId = -1;
                        }
                        onWallpaperSelected(wallpaperPath);
                        
                        // 如果一周壁纸设置区可见，启动拖拽
                        if (m_weeklyWallpaperVisible && widget->property("draggable").toBool()) {
                            QDrag* drag = new QDrag(widget);
                            QMimeData* mimeData = new QMimeData;
                            mimeData->setText(wallpaperPath);
                            drag->setMimeData(mimeData);
                            
                            // 设置拖拽时的预览图片
                            QPixmap pixmap = widget->grab();
                            drag->setPixmap(pixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            drag->setHotSpot(QPoint(50, 50));
                            
                            drag->exec(Qt::CopyAction);
                        }
                        
                        return true;
                    }
                } else if (widget->property("isAddButton").toBool()) {
                    onAddLocalWallpaperClicked();
                    return true;
                }
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

void WallpaperDialog::toggleWeeklyWallpaperSection()
{
    m_weeklyWallpaperVisible = !m_weeklyWallpaperVisible;
    
    if (m_weeklyWallpaperVisible) {
        m_weeklyWallpaperWidget->show();
        // 更新按钮样式，表示已激活
        m_weeklyWallpaperBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #608bd0;"
            "color: white;"
            "padding: 8px 16px;"
            "border-radius: 4px;"
            "font-weight: bold;"
            "}"
        );
    } else {
        m_weeklyWallpaperWidget->hide();
        // 恢复按钮样式
        m_weeklyWallpaperBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #555;"
            "color: white;"
            "padding: 8px 16px;"
            "border-radius: 4px;"
            "font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "background-color: #666;"
            "}"
        );
    }
}

void WallpaperDialog::setupWeeklyWallpaperSection()
{
    // 创建一周壁纸设置区容器
    m_weeklyWallpaperWidget = new QWidget(this);
    m_weeklyWallpaperWidget->setStyleSheet(
        "QWidget {"
        "background-color: #3a3a3a;"
        "border-radius: 8px;"
        "padding: 15px;"
        "}"
    );
    
    QVBoxLayout* weeklyLayout = new QVBoxLayout(m_weeklyWallpaperWidget);
    weeklyLayout->setContentsMargins(15, 15, 15, 15);
    weeklyLayout->setSpacing(15);
    
    // 标题
    QLabel* titleLabel = new QLabel("一周壁纸设置", m_weeklyWallpaperWidget);
    titleLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");
    weeklyLayout->addWidget(titleLabel);
    
    // 日期标签行
    QHBoxLayout* dayLabelsLayout = new QHBoxLayout;
    dayLabelsLayout->setSpacing(10);
    
    QStringList dayNames = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    for (int i = 0; i < 7; ++i) {
        QPushButton* dayBtn = new QPushButton(dayNames[i], m_weeklyWallpaperWidget);
        dayBtn->setFixedSize(100, 35);
        dayBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #4a9e4a;"
            "color: white;"
            "font-weight: bold;"
            "border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "background-color: #5aae5a;"
            "}"
        );
        dayLabelsLayout->addWidget(dayBtn);
    }
    weeklyLayout->addLayout(dayLabelsLayout);
    
    // 壁纸槽位行
    QHBoxLayout* wallpaperSlotsLayout = new QHBoxLayout;
    wallpaperSlotsLayout->setSpacing(10);
    
    for (int i = 0; i < 7; ++i) {
        QLabel* slotLabel = createWeeklyWallpaperSlot(i, dayNames[i]);
        wallpaperSlotsLayout->addWidget(slotLabel);
        m_weeklyWallpaperLabels[i] = slotLabel;
    }
    
    // 应用按钮
    m_applyWeeklyBtn = new QPushButton("应用", m_weeklyWallpaperWidget);
    m_applyWeeklyBtn->setFixedSize(80, 120);
    m_applyWeeklyBtn->setStyleSheet(
        "QPushButton {"
        "background-color: #d32f2f;"
        "color: white;"
        "font-weight: bold;"
        "font-size: 16px;"
        "border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "background-color: #e53935;"
        "}"
    );
    connect(m_applyWeeklyBtn, &QPushButton::clicked, this, &WallpaperDialog::onApplyWeeklyWallpaperClicked);
    wallpaperSlotsLayout->addWidget(m_applyWeeklyBtn);
    
    weeklyLayout->addLayout(wallpaperSlotsLayout);
    
    // 将一周壁纸设置区添加到主布局（在滚动区域之前）
    m_mainLayout->insertWidget(1, m_weeklyWallpaperWidget);
}

QLabel* WallpaperDialog::createWeeklyWallpaperSlot(int dayIndex, const QString& dayName)
{
    QLabel* slotLabel = new QLabel("图片", m_weeklyWallpaperWidget);
    slotLabel->setFixedSize(100, 120);
    slotLabel->setAlignment(Qt::AlignCenter);
    slotLabel->setStyleSheet(
        "QLabel {"
        "background-color: #555;"
        "border: 2px dashed #888;"
        "border-radius: 4px;"
        "color: #aaa;"
        "font-size: 14px;"
        "}"
    );
    slotLabel->setProperty("dayIndex", dayIndex);
    slotLabel->setProperty("isWeeklySlot", true);
    
    // 启用拖放 - 需要重写dragEnterEvent和dropEvent
    slotLabel->setAcceptDrops(true);
    slotLabel->installEventFilter(this);
    
    return slotLabel;
}

void WallpaperDialog::onApplyWeeklyWallpaperClicked()
{
    // 检查是否所有日期都设置了壁纸
    bool allSet = true;
    for (int i = 0; i < 7; ++i) {
        if (!m_weeklyWallpapers.contains(i) || m_weeklyWallpapers[i].isEmpty()) {
            allSet = false;
            break;
        }
    }
    
    if (!allSet) {
        QMessageBox::warning(this, "提示", "请为所有日期设置壁纸");
        return;
    }
    
    applyWeeklyWallpaper();
}

void WallpaperDialog::applyWeeklyWallpaper()
{
    if (m_httpHandler && !m_groupId.isEmpty()) {
        // 临时断开原有的信号连接
        disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
        disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
        
        // 连接应用一周壁纸专用的信号
        connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onApplyWeeklyWallpaperSuccess, Qt::UniqueConnection);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onApplyWeeklyWallpaperFailed, Qt::UniqueConnection);
        
        QJsonObject payload;
        payload["group_id"] = m_groupId;
        
        // 构建一周壁纸数据
        QJsonObject weeklyData;
        for (int i = 0; i < 7; ++i) {
            if (m_weeklyWallpapers.contains(i)) {
                weeklyData[QString::number(i)] = m_weeklyWallpapers[i];
            }
        }
        payload["weekly_wallpapers"] = weeklyData;
        
        QJsonDocument doc(payload);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QUrl url("http://47.100.126.194:5000/class-wallpapers/apply-weekly");
        m_httpHandler->post(url.toString(), jsonData);
    }
}

void WallpaperDialog::onApplyWeeklyWallpaperSuccess(const QString& resp)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onApplyWeeklyWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onApplyWeeklyWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject rootObj = doc.object();
        QJsonObject dataObj = rootObj.value("data").toObject();
        int code = dataObj.value("code").toInt(-1);
        QString message = dataObj.value("message").toString("应用成功");
        
        if (code == 200) {
            m_weeklyWallpaperEnabled = true;  // 应用成功后，标记为已启用
            QMessageBox::information(this, "提示", message);
            // 刷新班级壁纸列表
            loadClassWallpapers();
        } else {
            QMessageBox::warning(this, "提示", message);
        }
    } else {
        m_weeklyWallpaperEnabled = true;  // 应用成功后，标记为已启用
        QMessageBox::information(this, "提示", "一周壁纸应用成功");
        // 刷新班级壁纸列表
        loadClassWallpapers();
    }
}

void WallpaperDialog::onApplyWeeklyWallpaperFailed(const QString& err)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onApplyWeeklyWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onApplyWeeklyWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QMessageBox::critical(this, "错误", "应用一周壁纸失败：" + err);
}

bool WallpaperDialog::isWeeklyWallpaperEnabled()
{
    return m_weeklyWallpaperEnabled;
}

void WallpaperDialog::loadWeeklyWallpaperConfig()
{
    if (m_httpHandler && !m_groupId.isEmpty()) {
        // 临时断开原有的信号连接
        disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
        disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
        
        // 连接获取一周壁纸配置专用的信号
        connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onLoadWeeklyConfigSuccess, Qt::UniqueConnection);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onLoadWeeklyConfigFailed, Qt::UniqueConnection);
        
        QUrl url("http://47.100.126.194:5000/class-wallpapers/weekly-config");
        QUrlQuery query;
        query.addQueryItem("group_id", m_groupId);
        url.setQuery(query);
        m_httpHandler->get(url.toString());
    }
}

void WallpaperDialog::onLoadWeeklyConfigSuccess(const QString& resp)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onLoadWeeklyConfigSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onLoadWeeklyConfigFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject rootObj = doc.object();
        QJsonObject dataObj = rootObj.value("data").toObject();
        int code = dataObj.value("code").toInt(-1);
        
        if (code == 200) {
            m_weeklyWallpaperEnabled = dataObj.value("is_enabled").toBool(false);
            QJsonObject weeklyData = dataObj.value("weekly_wallpapers").toObject();
            
            // 如果已启用，加载配置到界面
            if (m_weeklyWallpaperEnabled && !weeklyData.isEmpty()) {
                m_weeklyWallpapers.clear();
                for (int i = 0; i < 7; ++i) {
                    QString dayKey = QString::number(i);
                    if (weeklyData.contains(dayKey)) {
                        QString wallpaperUrl = weeklyData.value(dayKey).toString();
                        if (!wallpaperUrl.isEmpty()) {
                            m_weeklyWallpapers[i] = wallpaperUrl;
                            // 更新界面显示
                            if (m_weeklyWallpaperLabels.contains(i)) {
                                updateWeeklyWallpaperSlot(i, wallpaperUrl);
                            }
                        }
                    }
                }
                
                // 显示一周壁纸设置区
                if (!m_weeklyWallpaperVisible) {
                    toggleWeeklyWallpaperSection();
                }
            } else {
                // 未启用，隐藏一周壁纸设置区
                if (m_weeklyWallpaperVisible) {
                    toggleWeeklyWallpaperSection();
                }
            }
        }
    }
}

void WallpaperDialog::onLoadWeeklyConfigFailed(const QString& err)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onLoadWeeklyConfigSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onLoadWeeklyConfigFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    qWarning() << "获取一周壁纸配置失败：" << err;
    // 失败时默认认为未启用
    m_weeklyWallpaperEnabled = false;
}

void WallpaperDialog::disableWeeklyWallpaper()
{
    if (m_httpHandler && !m_groupId.isEmpty()) {
        // 临时断开原有的信号连接
        disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
        disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
        
        // 连接停用一周壁纸专用的信号
        connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onDisableWeeklyWallpaperSuccess, Qt::UniqueConnection);
        connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onDisableWeeklyWallpaperFailed, Qt::UniqueConnection);
        
        QJsonObject payload;
        payload["group_id"] = m_groupId;
        
        QJsonDocument doc(payload);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QUrl url("http://47.100.126.194:5000/class-wallpapers/disable-weekly");
        m_httpHandler->post(url.toString(), jsonData);
    }
}

void WallpaperDialog::onDisableWeeklyWallpaperSuccess(const QString& resp)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onDisableWeeklyWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onDisableWeeklyWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
    
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject rootObj = doc.object();
        QJsonObject dataObj = rootObj.value("data").toObject();
        int code = dataObj.value("code").toInt(-1);
        
        if (code == 200) {
            m_weeklyWallpaperEnabled = false;
            // 停用成功后，继续设置单张壁纸
            if (m_selectedWallpaperId > 0) {
                setWallpaperToClass(m_selectedWallpaperId);
            }
        } else {
            QString message = dataObj.value("message").toString("停用一周壁纸失败");
            QMessageBox::warning(this, "提示", message);
        }
    } else {
        // 即使解析失败，也尝试继续设置壁纸
        m_weeklyWallpaperEnabled = false;
        if (m_selectedWallpaperId > 0) {
            setWallpaperToClass(m_selectedWallpaperId);
        }
    }
}

void WallpaperDialog::onDisableWeeklyWallpaperFailed(const QString& err)
{
    // 先恢复原有的信号连接
    disconnect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onDisableWeeklyWallpaperSuccess);
    disconnect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onDisableWeeklyWallpaperFailed);
    connect(m_httpHandler, &TAHttpHandler::success, this, &WallpaperDialog::onHttpSuccess);
    connect(m_httpHandler, &TAHttpHandler::failed, this, &WallpaperDialog::onHttpFailed);
    
    QMessageBox::critical(this, "错误", "停用一周壁纸失败：" + err);
}

void WallpaperDialog::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasText() || event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void WallpaperDialog::dropEvent(QDropEvent* event)
{
    // 获取拖放的目标位置
    QPoint pos = event->pos();
    QWidget* targetWidget = childAt(pos);
    
    // 如果直接点击在QLabel上，使用该QLabel
    if (!targetWidget) {
        // 尝试从位置查找一周壁纸槽位
        for (auto it = m_weeklyWallpaperLabels.begin(); it != m_weeklyWallpaperLabels.end(); ++it) {
            QLabel* slotLabel = it.value();
            if (slotLabel && slotLabel->geometry().contains(slotLabel->mapFromGlobal(mapToGlobal(pos)))) {
                targetWidget = slotLabel;
                break;
            }
        }
    }
    
    if (targetWidget) {
        QLabel* slotLabel = qobject_cast<QLabel*>(targetWidget);
        if (slotLabel && slotLabel->property("isWeeklySlot").toBool()) {
            int dayIndex = slotLabel->property("dayIndex").toInt();
            
            // 获取拖放的壁纸路径
            QString wallpaperPath;
            if (event->mimeData()->hasText()) {
                wallpaperPath = event->mimeData()->text();
            } else if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty()) {
                QUrl url = event->mimeData()->urls().first();
                if (url.isLocalFile()) {
                    wallpaperPath = url.toLocalFile();
                } else {
                    wallpaperPath = url.toString();
                }
            }
            
            if (!wallpaperPath.isEmpty()) {
                // 保存壁纸路径
                m_weeklyWallpapers[dayIndex] = wallpaperPath;
                
                // 更新标签显示
                updateWeeklyWallpaperSlot(dayIndex, wallpaperPath);
                
                event->acceptProposedAction();
                return;
            }
        }
    }
    
    event->ignore();
}

void WallpaperDialog::updateWeeklyWallpaperSlot(int dayIndex, const QString& wallpaperPath)
{
    if (!m_weeklyWallpaperLabels.contains(dayIndex)) {
        return;
    }
    
    QLabel* slotLabel = m_weeklyWallpaperLabels[dayIndex];
    
    // 加载并显示壁纸图片
    if (wallpaperPath.startsWith("http://") || wallpaperPath.startsWith("https://")) {
        // 网络图片，异步加载
        QUrl imageUrl(wallpaperPath);
        QNetworkRequest request(imageUrl);
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, [slotLabel, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QPixmap pixmap;
                pixmap.loadFromData(reply->readAll());
                if (!pixmap.isNull()) {
                    slotLabel->setPixmap(pixmap.scaled(100, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    slotLabel->setText("");
                }
            }
            reply->deleteLater();
        });
    } else {
        // 本地图片
        QPixmap pixmap(wallpaperPath);
        if (!pixmap.isNull()) {
            slotLabel->setPixmap(pixmap.scaled(100, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            slotLabel->setText("");
        }
    }
}

