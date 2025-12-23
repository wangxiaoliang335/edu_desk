#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QDate>
#include <QPainter>
#include <QPalette>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QMouseEvent>
#include <QEvent>
#include <QTableWidget>
#include <QResizeEvent>
#include <QMap>
#include <QStringList>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPushButton>
#include <QHBoxLayout>
#include "QXlsx/header/xlsxdocument.h"
#include "QXlsx/header/xlsxworksheet.h"
#include "QXlsx/header/xlsxcell.h"
#include "TAHttpHandler.h"
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>

class TACalendarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TACalendarWidget(QWidget* parent = nullptr);
    QDate selectedDate() const;
    void setSelectedDate(const QDate& date);
    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int val);
    void setRadius(int val);

signals:
    void selectionChanged();
protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
private slots:
    void onYearChanged(int year);
    void onMonthChanged(int month);
    
private:
    void updateCalendar();
    void createWeekdayLabels();
    void clearDateButtons();
    void populateYearCombo();
    void populateMonthCombo();
    void updateYearButton();
    void updateMonthButton();
   
private:
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;
    int m_radius;
    QDate m_currentDate;
    QDate m_selectedDate;
    QPushButton* m_yearButton;
    QPushButton* m_monthButton;
    QMenu* m_yearMenu;
    QMenu* m_monthMenu;
    QVBoxLayout* m_mainLayout;
    QGridLayout* m_calendarLayout;
    QList<QPushButton*> m_dateButtons;

    // 顶部悬浮关闭按钮 + 拖动移动
    QToolButton* m_closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
};

// 自定义提示窗口（无标题栏，显示在屏幕右下角）
class TACalendarToastWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    enum ToastType {
        Success,
        Warning,
        Error,
        Info
    };

    explicit TACalendarToastWidget(QWidget* parent = nullptr);
    static void showToast(QWidget* parent, const QString& message, ToastType type = Info, int duration = 3000);

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void positionAtBottomRight();
    void startFadeOut();

private:
    qreal m_opacity = 1.0;
    QString m_message;
    ToastType m_type;
    QTimer* m_timer = nullptr;
    QPropertyAnimation* m_fadeAnimation = nullptr;
};

// 学校学期校历（周次 × 星期 + 备注），用于展示"一个学校一个学期"的校历表
class TASchoolCalendarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TASchoolCalendarWidget(QWidget* parent = nullptr);

    void setBackgroundColor(const QColor& color);
    void setBorderColor(const QColor& color);
    void setBorderWidth(int val);
    void setRadius(int val);

    // 快速设置校历数据（后续可扩展为从JSON加载）
    void setCalendarInfo(const QString& schoolName,
                         const QString& termTitle,
                         const QDate& termStartMonday,
                         int weekCount,
                         const QString& remarkText,
                         const QSet<QDate>& holidayDates,
                         const QSet<QDate>& makeupWorkDates);

    // 新：按日期设置备注（会在表格里按周汇总显示）
    void setRemarksByDate(const QMap<QDate, QStringList>& remarksByDate);
    
    // 导入校历数据（从JSON文件）
    void importFromJson(const QString& jsonFilePath);
    
    // 导入校历数据（从Excel文件）
    void importFromExcel(const QString& excelFilePath);
    
    // 上传校历到服务器
    void uploadCalendarToServer();
    
    // 从服务器加载校历
    void loadCalendarFromServer(const QString& termTitle = QString(), const QString& startDate = QString());

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void rebuildTable();
    QString weekName(int idx) const;
    QString formatDateCell(const QDate& date) const;
    void applyCellStyle(QTableWidgetItem* item, const QDate& date) const;
    void parseCalendarResponse(const QString& response);
    void parseSingleCalendar(const QJsonObject& calendarObj);

private:
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth = 1;
    int m_radius = 15;

    QLabel* m_schoolLabel = nullptr;
    QLabel* m_termLabel = nullptr;
    QLabel* m_legendLabel = nullptr;
    QTableWidget* m_table = nullptr;
    QToolButton* m_closeButton = nullptr;

    // 拖动
    bool m_dragging = false;
    QPoint m_dragStartPos;

    // 数据
    QString m_schoolName;
    QString m_termTitle;
    QDate m_termStartMonday;
    int m_weekCount = 0;
    QString m_remarkText;
    QMap<QDate, QStringList> m_remarksByDate;
    QSet<QDate> m_holidayDates;
    QSet<QDate> m_makeupWorkDates;
    
    // HTTP处理器，用于上传校历
    TAHttpHandler* m_httpHandler = nullptr;
    
    // HTTP处理器，用于获取校历
    TAHttpHandler* m_httpHandlerForGet = nullptr;
};
