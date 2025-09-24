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
};
