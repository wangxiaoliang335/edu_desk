#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QList>
#include <QColor>
#include <algorithm>
//#include "ScheduleDialog.h" // 包含 StudentInfo 定义
#include "CommonInfo.h"
#include "HeatmapTypes.h" // 包含 SegmentRange 定义

class HeatmapViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeatmapViewDialog(QWidget* parent = nullptr);
    
    // 设置学生数据
    void setStudentData(const QList<struct StudentInfo>& students);
    
    // 设置分段区间（用于热力图1）
    void setSegments(const QList<struct SegmentRange>& segments);
    
    // 设置热力图类型（1=分段，2=渐变）
    void setHeatmapType(int type);

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onTypeChanged(int index);
    void onClose();

private:
    void drawSegmentHeatmap(QPainter& painter);
    void drawGradientHeatmap(QPainter& painter);
    QColor getColorForValue(double value);
    QColor getGradientColorForValue(double value);
    
    QComboBox* typeComboBox;
    QPushButton* btnClose;
    QList<struct StudentInfo> m_students;
    QList<struct SegmentRange> m_segments;
    int m_heatmapType; // 1=分段，2=渐变
    double m_minValue;
    double m_maxValue;
};

