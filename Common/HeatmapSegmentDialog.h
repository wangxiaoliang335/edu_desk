#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QColorDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QColor>
#include <QList>
#include "ScheduleDialog.h" // 包含 StudentInfo 定义
#include "HeatmapTypes.h" // 包含 SegmentRange 定义

class HeatmapSegmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeatmapSegmentDialog(QWidget* parent = nullptr);
    
    // 设置学生数据（用于计算人数和百分数）
    void setStudentData(const QList<struct StudentInfo>& students);
    
    // 获取分段区间列表
    QList<SegmentRange> getSegments() const;
    
    // 更新分段统计（重新计算人数和百分数）
    void updateStatistics();

private slots:
    void onAddRow();
    void onDeleteRow();
    void onColorClicked();
    void onConfirm();
    void onCellChanged(int row, int column);

private:
    void updateRowStatistics(int row);
    void updateAllStatistics();
    QColor getColorForRow(int row);
    void setColorForRow(int row, const QColor& color);
    
    QTableWidget* table;
    QPushButton* btnAddRow;
    QPushButton* btnConfirm;
    QList<struct StudentInfo> m_students;
    QList<SegmentRange> m_segments;
};

