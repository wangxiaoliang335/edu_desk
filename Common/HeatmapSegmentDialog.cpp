#include "HeatmapSegmentDialog.h"
#include <QHeaderView>
#include <QColorDialog>
#include <QMessageBox>

HeatmapSegmentDialog::HeatmapSegmentDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("分段区间设置");
    resize(800, 500);
    setStyleSheet("background-color: #e0f0ff; font-size:14px;");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 标题
    QLabel* lblTitle = new QLabel("期中成绩表");
    lblTitle->setStyleSheet("background-color: green; color: white; padding: 8px; border-radius: 4px; font-weight: bold;");
    lblTitle->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(lblTitle);

    // 表格
    table = new QTableWidget(3, 6);
    QStringList headers = { "最小值", "-", "最大值", "颜色", "人数", "百分数" };
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setStyleSheet(
        "QTableWidget { background-color: white; gridline-color: #ddd; }"
        "QTableWidget::item { padding: 5px; }"
        "QHeaderView::section { background-color: #4169e1; color: white; font-weight: bold; padding: 8px; }"
    );

    // 初始化默认分段：0-60, 60-80, 80-100
    for (int row = 0; row < 3; ++row) {
        // 最小值
        QTableWidgetItem* minItem = new QTableWidgetItem();
        if (row == 0) minItem->setText("0");
        else if (row == 1) minItem->setText("60");
        else minItem->setText("80");
        table->setItem(row, 0, minItem);

        // 分隔符（只读）
        QTableWidgetItem* sepItem = new QTableWidgetItem("-");
        sepItem->setFlags(sepItem->flags() & ~Qt::ItemIsEditable);
        sepItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 1, sepItem);

        // 最大值
        QTableWidgetItem* maxItem = new QTableWidgetItem();
        if (row == 0) maxItem->setText("60");
        else if (row == 1) maxItem->setText("80");
        else maxItem->setText("100");
        table->setItem(row, 2, maxItem);

        // 颜色按钮
        QPushButton* colorBtn = new QPushButton();
        QColor defaultColor;
        if (row == 0) defaultColor = QColor(255, 255, 200); // 浅黄色
        else if (row == 1) defaultColor = QColor(255, 165, 0); // 橙色
        else defaultColor = QColor(255, 0, 0); // 红色
        colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #ccc; min-width: 50px; min-height: 30px;").arg(defaultColor.name()));
        table->setCellWidget(row, 3, colorBtn);
        connect(colorBtn, &QPushButton::clicked, this, [this, row, colorBtn]() {
            // 从样式表中提取当前颜色
            QString style = colorBtn->styleSheet();
            QColor currentColor = QColor(255, 255, 200); // 默认颜色
            if (row == 0) currentColor = QColor(255, 255, 200);
            else if (row == 1) currentColor = QColor(255, 165, 0);
            else currentColor = QColor(255, 0, 0);
            
            QColor color = QColorDialog::getColor(currentColor, this, "选择颜色");
            if (color.isValid()) {
                colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #ccc; min-width: 50px; min-height: 30px;").arg(color.name()));
            }
        });

        // 人数（只读）
        QTableWidgetItem* countItem = new QTableWidgetItem("0");
        countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);
        countItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 4, countItem);

        // 百分数（只读）
        QTableWidgetItem* percentItem = new QTableWidgetItem("0%");
        percentItem->setFlags(percentItem->flags() & ~Qt::ItemIsEditable);
        percentItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 5, percentItem);
    }

    mainLayout->addWidget(table);

    // 按钮
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnAddRow = new QPushButton("+");
    btnAddRow->setStyleSheet("background-color: green; color: white; padding: 6px 12px; border-radius: 4px; font-size: 16px; font-weight: bold; min-width: 40px;");
    btnConfirm = new QPushButton("确定");
    btnConfirm->setStyleSheet("background-color: green; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px;");
    QPushButton* btnCancel = new QPushButton("取消");
    btnCancel->setStyleSheet("background-color: gray; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px;");

    btnLayout->addWidget(btnAddRow);
    btnLayout->addStretch();
    btnLayout->addWidget(btnConfirm);
    btnLayout->addWidget(btnCancel);

    mainLayout->addLayout(btnLayout);

    connect(btnAddRow, &QPushButton::clicked, this, &HeatmapSegmentDialog::onAddRow);
    connect(btnConfirm, &QPushButton::clicked, this, &HeatmapSegmentDialog::onConfirm);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(table, &QTableWidget::cellChanged, this, &HeatmapSegmentDialog::onCellChanged);
}

void HeatmapSegmentDialog::setStudentData(const QList<struct StudentInfo>& students)
{
    m_students = students;
    updateAllStatistics();
}

QList<SegmentRange> HeatmapSegmentDialog::getSegments() const
{
    return m_segments;
}

void HeatmapSegmentDialog::updateStatistics()
{
    updateAllStatistics();
}

void HeatmapSegmentDialog::onAddRow()
{
    int row = table->rowCount();
    table->insertRow(row);

    // 最小值
    QTableWidgetItem* minItem = new QTableWidgetItem("0");
    table->setItem(row, 0, minItem);

    // 分隔符
    QTableWidgetItem* sepItem = new QTableWidgetItem("-");
    sepItem->setFlags(sepItem->flags() & ~Qt::ItemIsEditable);
    sepItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, sepItem);

    // 最大值
    QTableWidgetItem* maxItem = new QTableWidgetItem("100");
    table->setItem(row, 2, maxItem);

    // 颜色按钮
    QPushButton* colorBtn = new QPushButton();
    QColor defaultColor = QColor(200, 200, 200); // 灰色
    colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #ccc; min-width: 50px; min-height: 30px;").arg(defaultColor.name()));
    table->setCellWidget(row, 3, colorBtn);
    connect(colorBtn, &QPushButton::clicked, this, [this, row, colorBtn]() {
        QColor color = QColorDialog::getColor(colorBtn->palette().button().color(), this, "选择颜色");
        if (color.isValid()) {
            colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #ccc; min-width: 50px; min-height: 30px;").arg(color.name()));
        }
    });

    // 人数
    QTableWidgetItem* countItem = new QTableWidgetItem("0");
    countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);
    countItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 4, countItem);

    // 百分数
    QTableWidgetItem* percentItem = new QTableWidgetItem("0%");
    percentItem->setFlags(percentItem->flags() & ~Qt::ItemIsEditable);
    percentItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 5, percentItem);

    updateRowStatistics(row);
}

void HeatmapSegmentDialog::onDeleteRow()
{
    int row = table->currentRow();
    if (row >= 0) {
        table->removeRow(row);
        updateAllStatistics();
    }
}

void HeatmapSegmentDialog::onColorClicked()
{
    // 颜色按钮的点击事件在创建时已经连接
}

void HeatmapSegmentDialog::onConfirm()
{
    // 读取所有分段区间
    m_segments.clear();
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* minItem = table->item(row, 0);
        QTableWidgetItem* maxItem = table->item(row, 2);
        QPushButton* colorBtn = qobject_cast<QPushButton*>(table->cellWidget(row, 3));
        QTableWidgetItem* countItem = table->item(row, 4);
        QTableWidgetItem* percentItem = table->item(row, 5);

        if (!minItem || !maxItem || !colorBtn) continue;

        bool ok1, ok2;
        double minValue = minItem->text().toDouble(&ok1);
        double maxValue = maxItem->text().toDouble(&ok2);

        if (!ok1 || !ok2) continue;

        SegmentRange segment;
        segment.minValue = minValue;
        segment.maxValue = maxValue;
        // 从样式表中提取颜色
        QString style = colorBtn->styleSheet();
        QColor btnColor = QColor(200, 200, 200); // 默认灰色
        int bgIndex = style.indexOf("background-color:");
        if (bgIndex >= 0) {
            int start = bgIndex + QString("background-color:").length();
            int end = style.indexOf(";", start);
            if (end < 0) end = style.indexOf(" ", start);
            if (end >= 0) {
                QString colorStr = style.mid(start, end - start).trimmed();
                btnColor = QColor(colorStr);
            }
        }
        segment.color = btnColor.isValid() ? btnColor : QColor(200, 200, 200);
        segment.count = countItem ? countItem->text().toInt() : 0;
        segment.percentage = percentItem ? percentItem->text().replace("%", "").toDouble() : 0.0;

        m_segments.append(segment);
    }

    accept();
}

void HeatmapSegmentDialog::onCellChanged(int row, int column)
{
    if (column == 0 || column == 2) {
        updateRowStatistics(row);
    }
}

void HeatmapSegmentDialog::updateRowStatistics(int row)
{
    QTableWidgetItem* minItem = table->item(row, 0);
    QTableWidgetItem* maxItem = table->item(row, 2);
    QTableWidgetItem* countItem = table->item(row, 4);
    QTableWidgetItem* percentItem = table->item(row, 5);

    if (!minItem || !maxItem || !countItem || !percentItem) return;

    bool ok1, ok2;
    double minValue = minItem->text().toDouble(&ok1);
    double maxValue = maxItem->text().toDouble(&ok2);

    if (!ok1 || !ok2) {
        countItem->setText("0");
        percentItem->setText("0%");
        return;
    }

    // 统计该区间内的学生数量
    int count = 0;
    for (const auto& student : m_students) {
        if (student.score >= minValue && student.score <= maxValue) {
            count++;
        }
    }

    countItem->setText(QString::number(count));

    // 计算百分数
    double percentage = 0.0;
    if (m_students.size() > 0) {
        percentage = (count * 100.0) / m_students.size();
    }
    percentItem->setText(QString::number(percentage, 'f', 1) + "%");
}

void HeatmapSegmentDialog::updateAllStatistics()
{
    for (int row = 0; row < table->rowCount(); ++row) {
        updateRowStatistics(row);
    }
}

QColor HeatmapSegmentDialog::getColorForRow(int row)
{
    QPushButton* colorBtn = qobject_cast<QPushButton*>(table->cellWidget(row, 3));
    if (colorBtn) {
        QString style = colorBtn->styleSheet();
        QColor btnColor = QColor(200, 200, 200);
        int bgIndex = style.indexOf("background-color:");
        if (bgIndex >= 0) {
            int start = bgIndex + QString("background-color:").length();
            int end = style.indexOf(";", start);
            if (end < 0) end = style.indexOf(" ", start);
            if (end >= 0) {
                QString colorStr = style.mid(start, end - start).trimmed();
                btnColor = QColor(colorStr);
            }
        }
        return btnColor.isValid() ? btnColor : QColor(200, 200, 200);
    }
    return QColor(200, 200, 200);
}

void HeatmapSegmentDialog::setColorForRow(int row, const QColor& color)
{
    QPushButton* colorBtn = qobject_cast<QPushButton*>(table->cellWidget(row, 3));
    if (colorBtn) {
        colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #ccc; min-width: 50px; min-height: 30px;").arg(color.name()));
    }
}

