#pragma once
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>
#include <QBrush>
#include <QStringList>

/**
 * 可编辑课程表控件
 * 支持横向星期列、纵向时间行、单元格编辑与高亮
 */
class CourseTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    CourseTableWidget(QWidget* parent = nullptr)
        : QTableWidget(parent),
        m_times(QStringList() << "6:00" << "8:10" << "8:50" << "9:45"
            << "10:40" << "11:35" << "14:10"
            << "15:05" << "15:50" << "19:00" << "20:00"),
        m_days(QStringList() << "周一" << "周二" << "周三"
            << "周四" << "周五" << "周六" << "周日")
    {

        // 初始化表格
        initTable();
    }

    // 初始化表格布局
    void initTable()
    {
        setRowCount(m_times.size());
        setColumnCount(m_days.size());

        setHorizontalHeaderLabels(m_days);
        setVerticalHeaderLabels(m_times);

        // 表格样式
        horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        setEditTriggers(QAbstractItemView::DoubleClicked); // 双击可编辑

        // 默认填充为空
        for (int row = 0; row < rowCount(); ++row) {
            for (int col = 0; col < columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                setItem(row, col, item);
            }
        }
    }

    // 设置课程
    void setCourse(int row, int col, const QString& name, bool highlight = false)
    {
        if (!validIndex(row, col)) return;

        QTableWidgetItem* item = this->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            setItem(row, col, item);
        }

        item->setText(name);
        item->setTextAlignment(Qt::AlignCenter);

        QFont font;
        font.setPointSize(12);
        item->setFont(font);

        if (highlight) {
            item->setBackground(QBrush(QColor(51, 153, 255))); // 蓝色背景
            item->setForeground(QBrush(Qt::white));           // 白色文字
        }
        else {
            item->setBackground(QBrush(Qt::white));
            item->setForeground(QBrush(Qt::black));
        }
    }

    // 获取课程
    QString courseAt(int row, int col) const
    {
        if (!validIndex(row, col)) return "";
        auto item = this->item(row, col);
        return item ? item->text() : "";
    }

private:
    QStringList m_times;
    QStringList m_days;

    bool validIndex(int row, int col) const
    {
        return row >= 0 && row < rowCount() && col >= 0 && col < columnCount();
    }
};
#pragma once
