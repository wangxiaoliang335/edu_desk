#pragma once
#pragma execution_character_set("utf-8")
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QPixmap>
#include <QMouseEvent>
#include <QPoint>
#include <QFile>
#include <QDir>
#include <QMap>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "TAHttpHandler.h"

class WallpaperDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WallpaperDialog(QWidget* parent = nullptr);
    ~WallpaperDialog();

    // 设置群组ID
    void setGroupId(const QString& groupId) {
        m_groupId = groupId;
    }

    // 设置班级ID
    void setClassId(const QString& classId) {
        m_classId = classId;
        loadClassWallpapers();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onCloseClicked();
    void onClassWallpaperClicked();
    void onWeeklyWallpaperClicked();
    void onSetWallpaperClicked();
    void onWallpaperLibraryClicked();
    void onBackClicked();
    void onDownloadClicked();
    void onAddLocalWallpaperClicked();
    void onWallpaperSelected(const QString& wallpaperPath);
    void onHttpSuccess(const QString& resp);
    void onHttpFailed(const QString& err);
    void onImageDownloaded(QNetworkReply* reply);
    void onDownloadWallpaperSuccess(const QString& resp);
    void onDownloadWallpaperFailed(const QString& err);
    void onSetWallpaperSuccess(const QString& resp);
    void onSetWallpaperFailed(const QString& err);
    void onApplyWeeklyWallpaperClicked();
    void onApplyWeeklyWallpaperSuccess(const QString& resp);
    void onApplyWeeklyWallpaperFailed(const QString& err);
    void onDisableWeeklyWallpaperSuccess(const QString& resp);
    void onDisableWeeklyWallpaperFailed(const QString& err);
    void onLoadWeeklyConfigSuccess(const QString& resp);
    void onLoadWeeklyConfigFailed(const QString& err);

private:
    void setupUI();
    void showClassWallpaperPage();
    void showWallpaperLibraryPage();
    void loadClassWallpapers();
    void loadWallpaperLibrary();
    void downloadWallpaperFromLibrary(int libraryWallpaperId);
    void setWallpaperToClass(int wallpaperId);
    void toggleWeeklyWallpaperSection();  // 切换一周壁纸设置区的显示/隐藏
    void setupWeeklyWallpaperSection();  // 设置一周壁纸设置区UI
    void applyWeeklyWallpaper();  // 应用一周壁纸设置
    void loadWeeklyWallpaperConfig();  // 加载一周壁纸配置
    void disableWeeklyWallpaper();  // 停用一周壁纸功能
    QWidget* createWallpaperThumbnail(const QString& imagePath, const QString& label, bool isSelectable = true);
    QLabel* createWeeklyWallpaperSlot(int dayIndex, const QString& dayName);  // 创建一周壁纸槽位
    void updateWeeklyWallpaperSlot(int dayIndex, const QString& wallpaperPath);  // 更新一周壁纸槽位显示
    void clearLayout(QLayout* layout);
    bool isWeeklyWallpaperEnabled();  // 检查一周壁纸是否启用

private:
    QPushButton* m_closeButton = nullptr;
    QPushButton* m_classWallpaperBtn = nullptr;  // 班级壁纸按钮
    QPushButton* m_weeklyWallpaperBtn = nullptr;
    QPushButton* m_setWallpaperBtn = nullptr;
    QPushButton* m_wallpaperLibraryBtn = nullptr;
    QPushButton* m_backButton = nullptr;
    QPushButton* m_downloadButton = nullptr;
    QPushButton* m_applyWeeklyBtn = nullptr;  // 应用一周壁纸按钮
    QLabel* m_wallpaperLibraryLabel = nullptr;  // 壁纸库标签（用于壁纸库页面显示）
    
    QVBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_topButtonLayout = nullptr;
    QWidget* m_weeklyWallpaperWidget = nullptr;  // 一周壁纸设置区（上栏）
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QGridLayout* m_gridLayout = nullptr;
    
    bool m_weeklyWallpaperVisible = false;  // 一周壁纸设置区是否可见
    bool m_weeklyWallpaperEnabled = false;  // 一周壁纸功能是否启用（从服务器获取）
    QMap<int, QString> m_weeklyWallpapers;  // 存储一周壁纸：dayIndex -> wallpaperPath
    QMap<int, QLabel*> m_weeklyWallpaperLabels;  // 存储一周壁纸标签：dayIndex -> label
    
    QString m_groupId;
    QString m_classId;
    QString m_selectedWallpaperPath;  // 壁纸图片URL
    int m_selectedWallpaperId = -1;   // 班级壁纸ID（用于设置当前壁纸）
    int m_selectedLibraryWallpaperId = -1;  // 壁纸库ID（用于下载）
    
    TAHttpHandler* m_httpHandler = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    
    bool m_dragging = false;
    QPoint m_dragStartPosition;
    
    enum PageType {
        ClassWallpaperPage,
        WallpaperLibraryPage
    };
    PageType m_currentPage = ClassWallpaperPage;
};

