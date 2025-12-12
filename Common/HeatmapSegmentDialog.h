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
#include <QComboBox>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPaintEvent>
//#include "ScheduleDialog.h" // 包含 StudentInfo 定义
#include "CommonInfo.h"
#include "HeatmapTypes.h" // 包含 SegmentRange 定义

// 自定义表格控件，用于绘制热力图背景
class HeatmapTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit HeatmapTableWidget(QWidget* parent = nullptr) : QTableWidget(parent) {}
    
    void setSegments(const QList<SegmentRange>& segments) {
        m_segments = segments;
        viewport()->update(); // 触发重绘
    }
    
protected:
    void paintEvent(QPaintEvent* event) override {
        // 先调用父类绘制表格内容
        QTableWidget::paintEvent(event);
        
        // 在表格背景上绘制热力图
        if (!m_segments.isEmpty()) {
            QPainter painter(viewport());
            painter.setRenderHint(QPainter::Antialiasing);
            
            // 绘制每行的热力图背景
            for (int row = 0; row < rowCount(); ++row) {
                QTableWidgetItem* firstItem = item(row, 0);
                if (!firstItem) continue;
                
                QRect rowRect = visualItemRect(firstItem);
                // 扩展行矩形以覆盖整行
                for (int col = 0; col < columnCount(); ++col) {
                    QTableWidgetItem* colItem = item(row, col);
                    if (colItem) {
                        QRect colRect = visualItemRect(colItem);
                        rowRect = rowRect.united(colRect);
                    }
                }
                
                if (rowRect.isValid()) {
                    // 获取该行对应的分段颜色
                    QColor rowColor = getRowColor(row);
                    if (rowColor.isValid()) {
                        // 绘制整行的背景色（渐变效果）
                        QLinearGradient gradient(rowRect.left(), rowRect.top(), rowRect.right(), rowRect.top());
                        gradient.setColorAt(0, rowColor);
                        gradient.setColorAt(1, rowColor.darker(110)); // 稍微变深
                        painter.fillRect(rowRect, gradient);
                    }
                }
            }
        }
    }
    
private:
    QList<SegmentRange> m_segments;
    
    QColor getRowColor(int row) const {
        if (row < 0 || row >= rowCount()) return QColor();
        
        // 获取该行的最小值（用于确定颜色）
        QTableWidgetItem* minItem = item(row, 0);
        if (!minItem) return QColor();
        
        bool ok;
        double minValue = minItem->text().toDouble(&ok);
        if (!ok) return QColor();
        
        // 根据最小值找到对应的分段颜色
        for (const auto& segment : m_segments) {
            if (minValue >= segment.minValue && minValue <= segment.maxValue) {
                return segment.color;
            }
        }
        
        return QColor();
    }
};

class HeatmapSegmentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HeatmapSegmentDialog(QWidget* parent = nullptr);
    
    // 设置学生数据（用于计算人数和百分数）
    void setStudentData(const QList<struct StudentInfo>& students);
    void setTableOptions(const QStringList& tables);
    void setAttributeOptions(const QStringList& attributes);
    QString selectedTable() const;
    QString selectedAttribute() const;
    
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
    void onTableChanged(int index); // 当表格选择改变时

private:
    void updateRowStatistics(int row);
    void updateAllStatistics();
    void updateTableHeatmap(); // 更新表格热力图显示
    QColor getColorForRow(int row);
    void setColorForRow(int row, const QColor& color);
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
    HeatmapTableWidget* table;
    QPushButton* btnAddRow;
    QPushButton* btnConfirm;
    QComboBox* tableComboBox;
    QComboBox* attributeComboBox;
    QPushButton* closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragStartPos;
    QList<struct StudentInfo> m_students;
    QList<SegmentRange> m_segments;
};

