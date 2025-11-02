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
#include <QPair>
#include <QMouseEvent>
#include <QEvent>
#include <QBrush>
#include <QColor>
#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <algorithm>

// 单元格注释窗口
class CellCommentWidget : public QWidget
{
    Q_OBJECT
public:
    CellCommentWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet(
            "QWidget { background-color: #ffa500; color: white; border: 1px solid #888; border-radius: 4px; padding: 8px; }"
        );
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        
        commentLabel = new QLabel("");
        commentLabel->setWordWrap(true);
        commentLabel->setStyleSheet("color: white; font-size: 12px;");
        layout->addWidget(commentLabel);
        
        hideTimer = new QTimer(this);
        hideTimer->setSingleShot(true);
        connect(hideTimer, &QTimer::timeout, this, &QWidget::hide);
    }
    
    void showComment(const QString& text, const QRect& cellRect, QWidget* parentWidget, int spanCols = 1)
    {
        commentLabel->setText(text.isEmpty() ? "(无注释)" : text);
        commentLabel->adjustSize();
        
        // 计算窗口大小
        int width = cellRect.width() * spanCols + 16;
        int height = commentLabel->height() + 16;
        resize(width, qMax(height, 40));
        
        // 计算窗口位置（在单元格上方）
        QPoint globalPos = parentWidget->mapToGlobal(cellRect.topLeft());
        int x = globalPos.x();
        int y = globalPos.y() - height - 5; // 在单元格上方5像素
        
        // 确保窗口不超出屏幕
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenRect = screen->geometry();
            if (x + width > screenRect.right()) {
                x = screenRect.right() - width;
            }
            if (x < screenRect.left()) {
                x = screenRect.left();
            }
            if (y < screenRect.top()) {
                y = globalPos.y() + cellRect.height() + 5; // 改为在单元格下方
            }
        }
        
        move(x, y);
        show();
    }
    
    void hideWithDelay(int ms = 3000)
    {
        hideTimer->start(ms);
    }
    
    void cancelHide()
    {
        hideTimer->stop();
    }
    
private:
    QLabel* commentLabel;
    QTimer* hideTimer;
};

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
        
        // 将表格添加到布局
        mainLayout->addWidget(table);
        
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
        
        // 创建注释窗口
        commentWidget = new CellCommentWidget(this);
        
        // 启用鼠标跟踪以显示悬浮提示
        table->setMouseTracking(true);
        
        // 右键菜单用于删除行和编辑注释
        table->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(table, &QTableWidget::customContextMenuRequested, this, &StudentPhysiqueDialog::onTableContextMenu);
        
        // 监听单元格内容变化，自动更新总分和小组总分
        connect(table, &QTableWidget::itemChanged, this, &StudentPhysiqueDialog::onItemChanged);
    }

    // 导入Excel数据
    void importData(const QStringList& headers, const QList<QStringList>& dataRows)
    {
        if (!table) return;

        // 清空现有数据
        table->setRowCount(0);

        // 获取列索引映射
        QMap<QString, int> headerMap;
        for (int i = 0; i < headers.size(); ++i) {
            headerMap[headers[i]] = i;
        }

        // 获取固定列的索引
        int colGroup = -1, colId = -1, colName = -1;
        QMap<QString, int> scoreColumnMap; // 存储其他评分列的映射
        int colTotal = -1, colGroupTotal = -1;
        
        // 在表格中找到对应的列
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
            if (!headerItem) continue;
            QString headerText = headerItem->text();
            
            if (headerText == "小组") colGroup = col;
            else if (headerText == "学号") colId = col;
            else if (headerText == "姓名") colName = col;
            else if (headerText == "总分") colTotal = col;
            else if (headerText == "小组总分") colGroupTotal = col;
            else {
                // 其他列可能是评分列
                if (headerMap.contains(headerText)) {
                    scoreColumnMap[headerText] = col;
                }
            }
        }

        // 导入数据
        QString currentGroup = "";
        int groupStartRow = -1;
        
        for (const QStringList& rowData : dataRows) {
            if (rowData.size() != headers.size()) continue; // 跳过列数不匹配的行

            int row = table->rowCount();
            table->insertRow(row);

            // 初始化所有单元格
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, col, item);
            }

            // 填充数据
            if (colGroup >= 0 && headerMap.contains("小组")) {
                int srcCol = headerMap["小组"];
                if (srcCol < rowData.size() && !rowData[srcCol].isEmpty()) {
                    QString group = rowData[srcCol];
                    table->item(row, colGroup)->setText(group);
                    currentGroup = group;
                    if (groupStartRow < 0) {
                        groupStartRow = row;
                    }
                } else if (!currentGroup.isEmpty()) {
                    table->item(row, colGroup)->setText(currentGroup);
                }
            }
            if (colId >= 0 && headerMap.contains("学号")) {
                int srcCol = headerMap["学号"];
                if (srcCol < rowData.size()) {
                    table->item(row, colId)->setText(rowData[srcCol]);
                }
            }
            if (colName >= 0 && headerMap.contains("姓名")) {
                int srcCol = headerMap["姓名"];
                if (srcCol < rowData.size()) {
                    table->item(row, colName)->setText(rowData[srcCol]);
                }
            }

            // 填充评分列
            for (auto it = scoreColumnMap.begin(); it != scoreColumnMap.end(); ++it) {
                QString headerName = it.key();
                int targetCol = it.value();
                if (headerMap.contains(headerName)) {
                    int srcCol = headerMap[headerName];
                    if (srcCol < rowData.size()) {
                        table->item(row, targetCol)->setText(rowData[srcCol]);
                    }
                }
            }
        }

        // 重新合并小组单元格并更新总分
        mergeGroupCells();
        updateAllTotals();
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
        if (obj == table) {
            if (event->type() == QEvent::MouseMove) {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                QTableWidgetItem* item = table->itemAt(mouseEvent->pos());
                if (item) {
                    int row = item->row();
                    int col = item->column();
                    
                    // 检查是否是跨列注释的起始单元格
                    int spanCols = item->data(Qt::UserRole + 1).toInt();
                    if (spanCols > 1) {
                        // 这是跨列注释的起始单元格
                        QString comment = item->data(Qt::UserRole).toString();
                        if (!comment.isEmpty() || true) { // 即使没有注释也显示窗口
                            QRect cellRect = table->visualItemRect(item);
                            // 计算跨列的宽度
                            int totalWidth = 0;
                            for (int i = 0; i < spanCols && col + i < table->columnCount(); ++i) {
                                totalWidth += table->columnWidth(col + i);
                            }
                            cellRect.setWidth(totalWidth);
                            
                            commentWidget->showComment(comment, cellRect, table, spanCols);
                            commentWidget->cancelHide();
                        }
                    } else {
                        // 检查是否是跨列注释的一部分
                        bool isInSpan = false;
                        QString comment = "";
                        int spanCols2 = 0;
                        for (int c = 0; c < col; ++c) {
                            QTableWidgetItem* checkItem = table->item(row, c);
                            if (checkItem) {
                                int sc = checkItem->data(Qt::UserRole + 1).toInt();
                                if (sc > 1 && c + sc > col) {
                                    // 当前单元格在跨列注释范围内
                                    isInSpan = true;
                                    comment = checkItem->data(Qt::UserRole).toString();
                                    spanCols2 = sc;
                                    break;
                                }
                            }
                        }
                        
                        if (isInSpan) {
                            QTableWidgetItem* startItem = table->item(row, col - 1);
                            for (int c = col - 1; c >= 0; --c) {
                                QTableWidgetItem* checkItem = table->item(row, c);
                                if (checkItem) {
                                    int sc = checkItem->data(Qt::UserRole + 1).toInt();
                                    if (sc > 1) {
                                        startItem = checkItem;
                                        comment = checkItem->data(Qt::UserRole).toString();
                                        spanCols2 = sc;
                                        break;
                                    }
                                }
                            }
                            
                            if (startItem) {
                                QRect cellRect = table->visualItemRect(startItem);
                                int startCol = table->column(startItem);
                                int totalWidth = 0;
                                for (int i = 0; i < spanCols2 && startCol + i < table->columnCount(); ++i) {
                                    totalWidth += table->columnWidth(startCol + i);
                                }
                                cellRect.setWidth(totalWidth);
                                commentWidget->showComment(comment, cellRect, table, spanCols2);
                                commentWidget->cancelHide();
                            }
                        } else {
                            // 单单元格注释
                            QString comment = item->data(Qt::UserRole).toString();
                            QRect cellRect = table->visualItemRect(item);
                            commentWidget->showComment(comment, cellRect, table, 1);
                            commentWidget->cancelHide();
                        }
                    }
                } else {
                    commentWidget->hideWithDelay(500);
                }
            } else if (event->type() == QEvent::Leave) {
                // 鼠标离开表格时，延迟隐藏注释窗口
                commentWidget->hideWithDelay(1000);
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
        
        // 设置示例数据 - 第一组（3行）
        // 第1行
        if (table->item(0, 0)) table->item(0, 0)->setText("1");
        if (table->item(0, 1)) table->item(0, 1)->setText("1");
        if (table->item(0, 3)) table->item(0, 3)->setText("100"); // 早读
        
        // 添加跨列注释示例：第1行的课堂发言和纪律列（列4-5）跨列注释
        QTableWidgetItem* commentItem0 = table->item(0, 4); // 课堂发言列
        if (commentItem0) {
            commentItem0->setData(Qt::UserRole, "声音大");
            commentItem0->setData(Qt::UserRole + 1, 2); // 跨2列
            commentItem0->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色标记
        }
        
        if (table->item(0, 8)) table->item(0, 8)->setText("123"); // 总分
        
        // 第2行
        if (table->item(1, 0)) table->item(1, 0)->setText("1");
        if (table->item(1, 1)) table->item(1, 1)->setText("2");
        if (table->item(1, 2)) table->item(1, 2)->setText("李四");
        if (table->item(1, 3)) table->item(1, 3)->setText("95"); // 早读
        if (table->item(1, 8)) table->item(1, 8)->setText("345"); // 总分
        
        // 第3行
        if (table->item(2, 0)) table->item(2, 0)->setText("1");
        if (table->item(2, 1)) table->item(2, 1)->setText("3");
        if (table->item(2, 2)) table->item(2, 2)->setText("张三");
        if (table->item(2, 3)) table->item(2, 3)->setText("75"); // 早读
        if (table->item(2, 4)) table->item(2, 4)->setText("80"); // 课堂发言
        
        // 添加跨列注释示例：第3行的纪律和作业列（列5-6）跨列注释
        QTableWidgetItem* commentItem1 = table->item(2, 5); // 纪律列
        if (commentItem1) {
            commentItem1->setData(Qt::UserRole, "积极踊跃");
            commentItem1->setData(Qt::UserRole + 1, 2); // 跨2列
            commentItem1->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色标记
        }
        
        if (table->item(2, 8)) table->item(2, 8)->setText("234"); // 总分
        
        // 设置示例数据 - 第二组（3行）
        // 第4行
        if (table->item(3, 0)) table->item(3, 0)->setText("2");
        if (table->item(3, 1)) table->item(3, 1)->setText("4");
        if (table->item(3, 3)) table->item(3, 3)->setText("67"); // 早读
        if (table->item(3, 4)) table->item(3, 4)->setText("97"); // 课堂发言
        
        // 第5行
        if (table->item(4, 0)) table->item(4, 0)->setText("2");
        if (table->item(4, 1)) table->item(4, 1)->setText("5");
        
        // 第6行
        if (table->item(5, 0)) table->item(5, 0)->setText("2");
        if (table->item(5, 1)) table->item(5, 1)->setText("6");
        
        // 合并小组单元格并计算总分
        mergeGroupCells();
        updateAllTotals();
        
        // 确保表格可见
        table->show();
        table->setVisible(true);
        
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
                // 检查该单元格是否被合并（可能是注释单元格）
                QTableWidgetItem* item = table->item(row, col);
                if (!item) continue;
                
                // 检查该单元格是否是跨列注释的起始单元格
                int spanCols = item->data(Qt::UserRole + 1).toInt();
                if (spanCols > 1) {
                    // 这是跨列注释的起始单元格，跳过（不计算注释单元格）
                    // 同时跳过该注释跨过的所有列
                    col += (spanCols - 1);
                    continue;
                }
                
                // 检查该单元格是否在某个跨列注释的范围内
                bool isInCommentSpan = false;
                for (int c = col - 1; c >= nameColumnIndex + 1 && c >= 0; --c) {
                    QTableWidgetItem* checkItem = table->item(row, c);
                    if (checkItem) {
                        int sc = checkItem->data(Qt::UserRole + 1).toInt();
                        if (sc > 1 && c + sc > col) {
                            // 当前单元格在跨列注释范围内
                            isInCommentSpan = true;
                            break;
                        }
                    }
                }
                if (isInCommentSpan) continue;
                
                QString text = item->text();
                bool ok;
                double value = text.toDouble(&ok);
                if (ok) {
                    total += value;
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
        int currentSpanCols = item->data(Qt::UserRole + 1).toInt();
        
        // 询问是否需要跨列注释
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("注释设置");
        msgBox.setText("选择注释显示方式：");
        QPushButton* singleBtn = msgBox.addButton("单单元格", QMessageBox::ActionRole);
        QPushButton* spanBtn = msgBox.addButton("跨列注释", QMessageBox::ActionRole);
        QPushButton* cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
        msgBox.exec();

        if (msgBox.clickedButton() == cancelBtn) {
            return;
        }

        bool isSpanMode = (msgBox.clickedButton() == spanBtn);
        int spanCols = currentSpanCols > 1 ? currentSpanCols : 1;
        
        if (isSpanMode) {
            bool ok;
            spanCols = QInputDialog::getInt(this, "跨列注释", 
                                            QString("请输入要跨越的列数（从当前列开始）:"),
                                            currentSpanCols > 1 ? currentSpanCols : 2, 
                                            1, table->columnCount() - column, 1, &ok);
            if (!ok) return;
        }

        bool ok;
        QString comment = QInputDialog::getMultiLineText(this, "单元格注释", 
                                                         QString("单元格 (%1, %2) 的注释:").arg(row + 1).arg(column + 1),
                                                         currentComment, &ok);
        if (ok) {
            // 清除该行之前的跨列注释
            clearSpanComment(row);
            
            if (!comment.isEmpty()) {
                // 存储注释（在起始单元格）
                item->setData(Qt::UserRole, comment);
                item->setData(Qt::UserRole + 1, spanCols); // 存储跨列数
                
                // 设置背景色标记（浅黄色，表示有注释）
                item->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色标记
                
                if (isSpanMode && spanCols > 1) {
                    // 不合并单元格，只标记起始单元格
                    // 注释通过悬浮窗口显示
                    item->setBackground(QBrush(QColor(255, 255, 200)));
                } else {
                    // 单单元格注释
                    item->setBackground(QBrush(QColor(255, 255, 200)));
                }
                
                // 重新合并小组单元格（保持小组合并）
                mergeGroupCells();
            } else {
                // 清除注释
                item->setData(Qt::UserRole, "");
                item->setData(Qt::UserRole + 1, 0);
                item->setBackground(QBrush()); // 恢复默认
                
                // 重新合并小组单元格
                mergeGroupCells();
            }
        }
    }

    void clearSpanComment(int row)
    {
        // 清除该行所有的跨列注释标记
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (item) {
                int spanCols = item->data(Qt::UserRole + 1).toInt();
                if (spanCols > 1) {
                    // 清除跨列注释标记
                    item->setData(Qt::UserRole, "");
                    item->setData(Qt::UserRole + 1, 0);
                    item->setBackground(QBrush()); // 恢复默认背景
                }
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
    CellCommentWidget* commentWidget = nullptr; // 注释窗口
};