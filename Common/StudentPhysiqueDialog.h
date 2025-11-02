#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QColorDialog>
#include <QMessageBox>
#include <QToolTip>
#include <QHeaderView>
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QSet>
#include <QTextStream>
#include <QFile>
#include <QIODevice>
#include <QLineEdit>
#include <QCursor>
#include <QMap>
#include <QVector>
#include <QMouseEvent>
#include <QEvent>
#include <QBrush>
#include <QColor>
#include <algorithm>

class StudentPhysiqueDialog : public QDialog
{
    Q_OBJECT

public:
    StudentPhysiqueDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("学生体质统计表");
        resize(1200, 800);
        setStyleSheet("background-color: #f5f5dc; font-size:14px;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(15, 15, 15, 15);

        // 标题
        QLabel* lblTitle = new QLabel("小组积分表");
        lblTitle->setStyleSheet("background-color: #add8e6; color: black; font-size: 16px; font-weight: bold; padding: 8px; border-radius: 4px;");
        lblTitle->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(lblTitle);

        // 顶部按钮行
        QHBoxLayout* btnLayout = new QHBoxLayout;
        btnLayout->setSpacing(8);

        QString btnStyle = "QPushButton { background-color: green; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; }"
                          "QPushButton:hover { background-color: #006400; }";

        btnAddRow = new QPushButton("添加行");
        btnDeleteColumn = new QPushButton("删除列");
        btnAddColumn = new QPushButton("添加列");
        btnFontColor = new QPushButton("字体颜色");
        btnBgColor = new QPushButton("背景色");
        btnExport = new QPushButton("导出");

        btnAddRow->setStyleSheet(btnStyle);
        btnDeleteColumn->setStyleSheet(btnStyle);
        btnAddColumn->setStyleSheet(btnStyle);
        btnFontColor->setStyleSheet(btnStyle);
        btnBgColor->setStyleSheet(btnStyle);
        btnExport->setStyleSheet(btnStyle);

        btnLayout->addWidget(btnAddRow);
        btnLayout->addWidget(btnDeleteColumn);
        btnLayout->addWidget(btnAddColumn);
        btnLayout->addWidget(btnFontColor);
        btnLayout->addWidget(btnBgColor);
        btnLayout->addWidget(btnExport);
        btnLayout->addStretch();

        mainLayout->addLayout(btnLayout);

        // 说明文本框
        QLabel* lblDesc = new QLabel("说明:");
        lblDesc->setStyleSheet("font-weight: bold; color: black;");
        mainLayout->addWidget(lblDesc);

        textDescription = new QTextEdit;
        textDescription->setPlaceholderText("在此处添加表格说明...");
        textDescription->setFixedHeight(60);
        textDescription->setStyleSheet("background-color: #ffa500; color: white; padding: 5px; border: 1px solid #888;");
        textDescription->setPlainText("该表为小组积分表。作业优秀为3,良好为2,未完成为0");
        mainLayout->addWidget(textDescription);

        // 表格
        // 固定列：小组(0)、学号(1)、姓名(2)、总分(总分数-2)、小组总分(总分数-1)
        // 可添加列在姓名后插入
        table = new QTableWidget(6, 10); // 初始6行，10列（小组、学号、姓名、早读、课堂发言、纪律、作业、背诵、总分、小组总分）
        QStringList headers = { "小组", "学号", "姓名", "早读", "课堂发言", "纪律", "作业", "背诵", "总分", "小组总分" };
        table->setHorizontalHeaderLabels(headers);

        // 表格样式
        table->setStyleSheet(
            "QTableWidget { background-color: white; gridline-color: #ddd; }"
            "QTableWidget::item { padding: 5px; }"
            "QHeaderView::section { background-color: #4169e1; color: white; font-weight: bold; padding: 8px; }"
        );
        table->setAlternatingRowColors(true);
        table->setStyleSheet(table->styleSheet() + 
            "QTableWidget { alternate-background-color: #e6f3ff; }"
        );

        table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
        table->setSelectionBehavior(QAbstractItemView::SelectItems);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        
        // 禁止列移动
        table->horizontalHeader()->setSectionsMovable(false);

        // 固定列索引（不能删除的列）
        // 小组(0)、学号(1)、姓名(2)、总分(总分数-2)、小组总分(总分数-1)
        fixedColumns = { 0, 1, 2 }; // 小组、学号、姓名
        nameColumnIndex = 2; // 姓名列索引
        groupColumnIndex = 0; // 小组列索引
        
        // 初始化表格数据并合并小组单元格
        initializeTableData();
        
        // 连接信号和槽
        connect(btnAddRow, &QPushButton::clicked, this, &StudentPhysiqueDialog::onAddRow);
        connect(btnDeleteColumn, &QPushButton::clicked, this, &StudentPhysiqueDialog::onDeleteColumn);
        connect(btnAddColumn, &QPushButton::clicked, this, &StudentPhysiqueDialog::onAddColumn);
        connect(btnFontColor, &QPushButton::clicked, this, &StudentPhysiqueDialog::onFontColor);
        connect(btnBgColor, &QPushButton::clicked, this, &StudentPhysiqueDialog::onBgColor);
        connect(btnExport, &QPushButton::clicked, this, &StudentPhysiqueDialog::onExport);

        // 单元格点击和悬浮事件
        connect(table, &QTableWidget::cellClicked, this, &StudentPhysiqueDialog::onCellClicked);
        connect(table, &QTableWidget::cellChanged, this, &StudentPhysiqueDialog::onCellChanged);
        
        // 启用鼠标跟踪以显示悬浮提示
        table->setMouseTracking(true);
        
        // 右键菜单用于删除行和编辑注释
        table->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(table, &QTableWidget::customContextMenuRequested, this, &StudentPhysiqueDialog::onTableContextMenu);
        
        // 监听单元格内容变化，自动更新总分和小组总分
        connect(table, &QTableWidget::itemChanged, this, &StudentPhysiqueDialog::onItemChanged);
    }

private slots:
    void onAddRow()
    {
        int currentRow = table->currentRow();
        if (currentRow < 0) {
            currentRow = table->rowCount() - 1;
        }
        if (currentRow < 0) {
            currentRow = 0;
        }
        
        // 获取当前行所属的小组
        QTableWidgetItem* groupItem = table->item(currentRow, groupColumnIndex);
        QString groupName = groupItem ? groupItem->text() : "";
        
        int insertRow = currentRow + 1;
        table->insertRow(insertRow);

        // 初始化新行的所有单元格
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(insertRow, col, item);
        }
        
        // 设置新行的小组名称（与当前行相同）
        if (!groupName.isEmpty()) {
            QTableWidgetItem* newGroupItem = table->item(insertRow, groupColumnIndex);
            if (newGroupItem) {
                newGroupItem->setText(groupName);
            }
        }
        
        // 重新合并小组单元格
        mergeGroupCells();
        // 更新小组总分
        updateGroupTotals();
    }

    void onDeleteColumn()
    {
        int currentCol = table->currentColumn();
        if (currentCol < 0) {
            QMessageBox::information(this, "提示", "请先选择要删除的列");
            return;
        }

        // 检查是否是固定列
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        int groupTotalScoreCol = totalColumns - 1; // 小组总分列
        
        if (fixedColumns.contains(currentCol) || currentCol == totalScoreCol || currentCol == groupTotalScoreCol) {
            QMessageBox::warning(this, "警告", "不能删除固定列（小组、学号、姓名、总分、小组总分）");
            return;
        }

        int ret = QMessageBox::question(this, "确认", "确定要删除这一列吗？", 
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            table->removeColumn(currentCol);
            
            // 更新固定列索引（删除列后，后续列的索引会变化）
            QSet<int> newFixedColumns;
            for (int col : fixedColumns) {
                if (col < currentCol) {
                    newFixedColumns.insert(col);
                } else if (col > currentCol) {
                    newFixedColumns.insert(col - 1);
                }
            }
            fixedColumns = newFixedColumns;
            
            // 更新姓名列索引
            if (nameColumnIndex > currentCol) {
                nameColumnIndex--;
            }
            if (groupColumnIndex > currentCol) {
                groupColumnIndex--;
            }
            
            // 重新合并小组单元格并更新总分
            mergeGroupCells();
            updateAllTotals();
        }
    }

    void onAddColumn()
    {
        bool ok;
        QString columnName = QInputDialog::getText(this, "添加列", "请输入列名:", 
                                                   QLineEdit::Normal, "", &ok);
        if (!ok || columnName.isEmpty()) {
            return;
        }

        // 在姓名列后添加（但要在总分列之前）
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        
        int insertCol = nameColumnIndex + 1;
        if (insertCol >= totalScoreCol) {
            insertCol = totalScoreCol; // 确保在总分列之前
        }
        
        table->insertColumn(insertCol);
        table->setHorizontalHeaderItem(insertCol, new QTableWidgetItem(columnName));

        // 初始化新列的所有单元格
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, insertCol, item);
        }

        // 更新固定列索引（插入列后，姓名列之后的列索引会变化）
        QSet<int> newFixedColumns;
        for (int col : fixedColumns) {
            if (col <= nameColumnIndex) {
                newFixedColumns.insert(col); // 姓名列及其之前的列索引不变
            } else {
                newFixedColumns.insert(col + 1); // 姓名列之后的列索引+1
            }
        }
        fixedColumns = newFixedColumns;
        
        // 重新合并小组单元格（因为列索引变化了）
        mergeGroupCells();
    }

    void onFontColor()
    {
        QTableWidgetItem* item = table->currentItem();
        if (!item) {
            QMessageBox::information(this, "提示", "请先选择要设置字体颜色的单元格");
            return;
        }

        QColor color = QColorDialog::getColor(item->foreground().color(), this, "选择字体颜色");
        if (color.isValid()) {
            item->setForeground(QBrush(color));
        }
    }

    void onBgColor()
    {
        QTableWidgetItem* item = table->currentItem();
        if (!item) {
            QMessageBox::information(this, "提示", "请先选择要设置背景色的单元格");
            return;
        }

        QColor color = QColorDialog::getColor(item->background().color(), this, "选择背景色");
        if (color.isValid()) {
            item->setBackground(QBrush(color));
        }
    }

    void onExport()
    {
        QString fileName = QFileDialog::getSaveFileName(this, "导出统计表", "", "CSV文件 (*.csv);;所有文件 (*.*)");
        if (fileName.isEmpty()) {
            return;
        }

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "错误", "无法创建文件");
            return;
        }

        QTextStream out(&file);
        out.setCodec("UTF-8");
        
        // 写入UTF-8 BOM以支持Excel正确显示中文
        out << "\xEF\xBB\xBF";

        // 导出说明
        QString desc = textDescription->toPlainText();
        if (!desc.isEmpty()) {
            out << desc << "\n\n";
        }

        // 导出表头
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
            QString headerText = headerItem ? headerItem->text() : "";
            if (!headerText.isEmpty()) {
                out << headerText;
            }
            if (col < table->columnCount() - 1) {
                out << ",";
            }
        }
        out << "\n";

        // 导出数据
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = table->item(row, col);
                QString text = item ? item->text() : "";
                // 处理CSV中的逗号和引号
                if (text.contains(",") || text.contains("\"") || text.contains("\n")) {
                    text = "\"" + text.replace("\"", "\"\"") + "\"";
                }
                out << text;
                if (col < table->columnCount() - 1) {
                    out << ",";
                }
            }
            out << "\n";
        }

        file.close();
        QMessageBox::information(this, "成功", "导出完成！");
    }

    void onCellClicked(int row, int column)
    {
        // 点击单元格时显示注释编辑对话框
        showCellComment(row, column);
    }

    void onCellChanged(int row, int column)
    {
        // 单元格内容改变时，更新总分和小组总分
        updateAllTotals();
        mergeGroupCells();
    }

    void onItemChanged(QTableWidgetItem* item)
    {
        if (!item) return;
        
        // 如果修改的是小组列，需要重新合并单元格
        if (item->column() == groupColumnIndex) {
            mergeGroupCells();
            updateGroupTotals();
        } else {
            // 其他列改变时，更新总分
            updateAllTotals();
        }
    }

    void onTableContextMenu(const QPoint& pos)
    {
        QTableWidgetItem* item = table->itemAt(pos);
        if (!item) return;

        int row = item->row();
        QMenu menu(this);
        
        QAction* editCommentAction = menu.addAction("编辑注释");
        QAction* deleteRowAction = menu.addAction("删除行");

        QAction* selectedAction = menu.exec(table->viewport()->mapToGlobal(pos));
        
        if (selectedAction == editCommentAction) {
            showCellComment(row, item->column());
        } else if (selectedAction == deleteRowAction) {
            int ret = QMessageBox::question(this, "确认", "确定要删除这一行吗？删除后该小组将少一人。", 
                                            QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                table->removeRow(row);
                mergeGroupCells();
                updateGroupTotals();
            }
        }
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (obj == table && event->type() == QEvent::MouseMove) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QTableWidgetItem* item = table->itemAt(mouseEvent->pos());
            if (item) {
                QString comment = item->data(Qt::UserRole).toString();
                if (!comment.isEmpty()) {
                    QToolTip::showText(mouseEvent->globalPos(), comment, table);
                } else {
                    QToolTip::hideText();
                }
            }
        }
        return QDialog::eventFilter(obj, event);
    }

private:
    void initializeTableData()
    {
        // 初始化示例数据
        for (int row = 0; row < table->rowCount(); ++row) {
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, col, item);
            }
        }
        
        // 设置示例数据 - 第一组
        if (table->item(0, 0)) table->item(0, 0)->setText("1组");
        if (table->item(0, 1)) table->item(0, 1)->setText("1");
        if (table->item(0, 3)) table->item(0, 3)->setText("100");
        
        if (table->item(1, 0)) table->item(1, 0)->setText("1组");
        if (table->item(1, 1)) table->item(1, 1)->setText("2");
        if (table->item(1, 2)) table->item(1, 2)->setText("李四");
        if (table->item(1, 3)) table->item(1, 3)->setText("95");
        
        if (table->item(2, 0)) table->item(2, 0)->setText("1组");
        if (table->item(2, 1)) table->item(2, 1)->setText("3");
        if (table->item(2, 2)) table->item(2, 2)->setText("张三");
        if (table->item(2, 3)) table->item(2, 3)->setText("75");
        if (table->item(2, 4)) table->item(2, 4)->setText("80");
        
        // 设置示例数据 - 第二组
        if (table->item(3, 0)) table->item(3, 0)->setText("2组");
        if (table->item(3, 1)) table->item(3, 1)->setText("4");
        if (table->item(3, 3)) table->item(3, 3)->setText("67");
        if (table->item(3, 4)) table->item(3, 4)->setText("97");
        
        if (table->item(4, 0)) table->item(4, 0)->setText("2组");
        if (table->item(5, 0)) table->item(5, 0)->setText("2组");
        
        // 合并小组单元格并计算总分
        mergeGroupCells();
        updateAllTotals();
        
        // 安装事件过滤器以处理鼠标悬浮
        table->installEventFilter(this);
    }

    void mergeGroupCells()
    {
        // 清除所有合并
        table->clearSpans();
        
        int groupCol = groupColumnIndex;
        if (groupCol < 0 || groupCol >= table->columnCount()) return;
        
        QString currentGroup;
        int startRow = -1;
        
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* item = table->item(row, groupCol);
            QString groupName = item ? item->text() : "";
            
            if (groupName.isEmpty()) {
                // 如果当前行小组为空，结束上一个合并
                if (startRow >= 0 && row > startRow) {
                    table->setSpan(startRow, groupCol, row - startRow, 1);
                }
                startRow = -1;
                currentGroup = "";
                continue;
            }
            
            if (groupName != currentGroup) {
                // 如果小组名称变化，先合并上一个小组
                if (startRow >= 0 && row > startRow) {
                    table->setSpan(startRow, groupCol, row - startRow, 1);
                }
                // 开始新小组
                startRow = row;
                currentGroup = groupName;
            }
        }
        
        // 处理最后一组
        if (startRow >= 0 && table->rowCount() > startRow) {
            int endRow = table->rowCount();
            table->setSpan(startRow, groupCol, endRow - startRow, 1);
        }
    }

    void updateAllTotals()
    {
        // 更新所有行的个人总分
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        
        for (int row = 0; row < table->rowCount(); ++row) {
            double total = 0.0;
            
            // 从姓名列之后到总分列之前的所有列都是计分列
            for (int col = nameColumnIndex + 1; col < totalScoreCol; ++col) {
                QTableWidgetItem* item = table->item(row, col);
                if (item) {
                    QString text = item->text();
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) {
                        total += value;
                    }
                }
            }
            
            // 设置总分
            QTableWidgetItem* totalItem = table->item(row, totalScoreCol);
            if (!totalItem) {
                totalItem = new QTableWidgetItem("");
                totalItem->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, totalScoreCol, totalItem);
            }
            totalItem->setText(QString::number(total));
        }
        
        // 更新小组总分
        updateGroupTotals();
    }

    void updateGroupTotals()
    {
        int totalColumns = table->columnCount();
        int totalScoreCol = totalColumns - 2; // 总分列
        int groupTotalScoreCol = totalColumns - 1; // 小组总分列
        int groupCol = groupColumnIndex;
        
        // 按小组汇总总分
        QMap<QString, double> groupTotals;
        QMap<QString, QVector<int>> groupRows;
        
        // 收集每个小组的行号和总分
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem* groupItem = table->item(row, groupCol);
            QTableWidgetItem* totalItem = table->item(row, totalScoreCol);
            
            if (groupItem && totalItem) {
                QString groupName = groupItem->text();
                if (!groupName.isEmpty()) {
                    QString totalText = totalItem->text();
                    bool ok;
                    double total = totalText.toDouble(&ok);
                    if (ok) {
                        groupTotals[groupName] += total;
                        if (!groupRows.contains(groupName)) {
                            groupRows[groupName] = QVector<int>();
                        }
                        groupRows[groupName].append(row);
                    }
                }
            }
        }
        
        // 设置每个小组的小组总分（只在每个小组的第一行显示）
        for (auto it = groupRows.begin(); it != groupRows.end(); ++it) {
            QString groupName = it.key();
            QVector<int> rows = it.value();
            
            if (!rows.isEmpty()) {
                double groupTotal = groupTotals[groupName];
                
                // 只在该小组的第一行显示小组总分
                int firstRow = rows[0];
                QTableWidgetItem* groupTotalItem = table->item(firstRow, groupTotalScoreCol);
                if (!groupTotalItem) {
                    groupTotalItem = new QTableWidgetItem("");
                    groupTotalItem->setTextAlignment(Qt::AlignCenter);
                    table->setItem(firstRow, groupTotalScoreCol, groupTotalItem);
                }
                groupTotalItem->setText(QString::number(groupTotal));
                
                // 合并小组总分列
                if (rows.size() > 1) {
                    table->setSpan(firstRow, groupTotalScoreCol, rows.size(), 1);
                }
                
                // 清除其他行的小组总分显示
                for (int i = 1; i < rows.size(); ++i) {
                    QTableWidgetItem* item = table->item(rows[i], groupTotalScoreCol);
                    if (item) {
                        item->setText("");
                    }
                }
            }
        }
    }

    void showCellComment(int row, int column)
    {
        QTableWidgetItem* item = table->item(row, column);
        if (!item) {
            item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, column, item);
        }

        QString currentComment = item->data(Qt::UserRole).toString();
        bool ok;
        QString comment = QInputDialog::getMultiLineText(this, "单元格注释", 
                                                         QString("单元格 (%1, %2) 的注释:").arg(row + 1).arg(column + 1),
                                                         currentComment, &ok);
        if (ok) {
            item->setData(Qt::UserRole, comment);
            // 如果有注释，用特殊背景色标记
            if (!comment.isEmpty()) {
                item->setBackground(QBrush(QColor(255, 200, 150))); // 橙色标记
            } else {
                item->setBackground(QBrush()); // 恢复默认
            }
        }
    }

private:
    QTableWidget* table;
    QTextEdit* textDescription;
    QPushButton* btnAddRow;
    QPushButton* btnDeleteColumn;
    QPushButton* btnAddColumn;
    QPushButton* btnFontColor;
    QPushButton* btnBgColor;
    QPushButton* btnExport;
    
    QSet<int> fixedColumns; // 固定列索引集合（不能删除的列）
    int nameColumnIndex; // 姓名列索引
    int groupColumnIndex; // 小组列索引
};