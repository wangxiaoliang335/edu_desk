#include "HeatmapViewDialog.h"
#include <QPainter>
#include <algorithm>

HeatmapViewDialog::HeatmapViewDialog(QWidget* parent)
    : QDialog(parent)
    , m_heatmapType(2)
    , m_minValue(0)
    , m_maxValue(100)
{
    setWindowTitle("热力图");
    resize(800, 600);
    setStyleSheet("background-color: #f5f5f5;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 顶部控制栏
    QHBoxLayout* controlLayout = new QHBoxLayout;
    QLabel* lblType = new QLabel("热力图类型:");
    typeComboBox = new QComboBox;
    typeComboBox->addItem("分段图1（每一段一种颜色）");
    typeComboBox->addItem("热力图2（颜色渐变）");
    typeComboBox->setCurrentIndex(1); // 默认渐变

    controlLayout->addWidget(lblType);
    controlLayout->addWidget(typeComboBox);
    controlLayout->addStretch();

    btnClose = new QPushButton("×");
    btnClose->setStyleSheet("background-color: red; color: white; padding: 6px 12px; border-radius: 4px; font-size: 16px; font-weight: bold; min-width: 30px;");
    controlLayout->addWidget(btnClose);

    mainLayout->addLayout(controlLayout);

    connect(typeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &HeatmapViewDialog::onTypeChanged);
    connect(btnClose, &QPushButton::clicked, this, &HeatmapViewDialog::onClose);
}

void HeatmapViewDialog::setStudentData(const QList<struct StudentInfo>& students)
{
    m_students = students;
    
    // 计算最小和最大成绩
    if (!m_students.isEmpty()) {
        m_minValue = m_students[0].score;
        m_maxValue = m_students[0].score;
        for (const auto& student : m_students) {
            if (student.score < m_minValue) m_minValue = student.score;
            if (student.score > m_maxValue) m_maxValue = student.score;
        }
    }
    
    update();
}

void HeatmapViewDialog::setSegments(const QList<struct SegmentRange>& segments)
{
    m_segments = segments;
    update();
}

void HeatmapViewDialog::setHeatmapType(int type)
{
    m_heatmapType = type;
    typeComboBox->setCurrentIndex(type - 1);
    update();
}

void HeatmapViewDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (m_heatmapType == 1) {
        drawSegmentHeatmap(painter);
    } else {
        drawGradientHeatmap(painter);
    }
}

void HeatmapViewDialog::drawSegmentHeatmap(QPainter& painter)
{
    // 绘制分段热力图
    int width = this->width() - 30;
    int height = this->height() - 100;
    int startX = 15;
    int startY = 80;
    
    // 计算每个学生的位置（简化：按成绩排序后排列）
    QList<struct StudentInfo> sortedStudents = m_students;
    std::sort(sortedStudents.begin(), sortedStudents.end(), 
        [](const struct StudentInfo& a, const struct StudentInfo& b) {
            return a.score < b.score;
        });
    
    int cols = 20; // 每行20个
    int cellWidth = width / cols;
    int cellHeight = cellWidth;
    
    for (int i = 0; i < sortedStudents.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        int x = startX + col * cellWidth;
        int y = startY + row * cellHeight;
        
        QColor color = getColorForValue(sortedStudents[i].score);
        painter.fillRect(x, y, cellWidth - 2, cellHeight - 2, color);
    }
}

void HeatmapViewDialog::drawGradientHeatmap(QPainter& painter)
{
    // 绘制渐变热力图
    int width = this->width() - 30;
    int height = this->height() - 100;
    int startX = 15;
    int startY = 80;
    
    // 计算每个学生的位置
    QList<struct StudentInfo> sortedStudents = m_students;
    std::sort(sortedStudents.begin(), sortedStudents.end(), 
        [](const struct StudentInfo& a, const struct StudentInfo& b) {
            return a.score < b.score;
        });
    
    int cols = 20;
    int cellWidth = width / cols;
    int cellHeight = cellWidth;
    
    for (int i = 0; i < sortedStudents.size(); ++i) {
        int row = i / cols;
        int col = i % cols;
        int x = startX + col * cellWidth;
        int y = startY + row * cellHeight;
        
        QColor color = getGradientColorForValue(sortedStudents[i].score);
        painter.fillRect(x, y, cellWidth - 2, cellHeight - 2, color);
    }
}

QColor HeatmapViewDialog::getColorForValue(double value)
{
    // 根据分段区间返回颜色
    for (const auto& segment : m_segments) {
        if (value >= segment.minValue && value <= segment.maxValue) {
            return segment.color;
        }
    }
    return QColor(200, 200, 200); // 默认灰色
}

QColor HeatmapViewDialog::getGradientColorForValue(double value)
{
    // 计算归一化值 (0-1)
    double range = m_maxValue - m_minValue;
    if (range == 0) range = 1;
    double normalized = (value - m_minValue) / range;
    
    // 计算渐变颜色：蓝色(低) -> 绿色 -> 黄色 -> 红色(高)
    QColor color;
    if (normalized < 0.33) {
        double t = normalized / 0.33;
        color = QColor(
            int(0 + t * 0),
            int(0 + t * 255),
            int(255 - t * 0)
        );
    } else if (normalized < 0.66) {
        double t = (normalized - 0.33) / 0.33;
        color = QColor(
            int(0 + t * 255),
            255,
            int(255 - t * 255)
        );
    } else {
        double t = (normalized - 0.66) / 0.34;
        color = QColor(
            255,
            int(255 - t * 255),
            0
        );
    }
    
    return color;
}

void HeatmapViewDialog::onTypeChanged(int index)
{
    m_heatmapType = index + 1;
    update();
}

void HeatmapViewDialog::onClose()
{
    close();
}

