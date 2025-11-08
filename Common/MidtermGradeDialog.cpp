#include "MidtermGradeDialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QToolTip>
#include <QCursor>
#include <QMenu>
#include <QAction>
#include <QBrush>
#include <QColor>
#include <algorithm>
#include "ScheduleDialog.h"
#include "ArrangeSeatDialog.h"
#include <QApplication>
#include <QDebug>
#include <QDate>

MidtermGradeDialog::MidtermGradeDialog(QString classid, QWidget* parent) : QDialog(parent)
{
    setWindowTitle("期中成绩表");
    resize(1200, 800);
    setStyleSheet("background-color: #f5f5dc; font-size:14px;");

    m_classid = classid;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 标题
    QLabel* lblTitle = new QLabel("期中成绩表");
    lblTitle->setStyleSheet("background-color: #d3d3d3; color: black; font-size: 16px; font-weight: bold; padding: 8px; border-radius: 4px;");
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
    btnDescOrder = new QPushButton("倒序");
    btnAscOrder = new QPushButton("正序");
    btnExport = new QPushButton("导出");
    btnUpload = new QPushButton("上传服务器");
    qDebug() << "创建上传按钮，按钮指针:" << btnUpload;

    btnAddRow->setStyleSheet(btnStyle);
    btnDeleteColumn->setStyleSheet(btnStyle);
    btnAddColumn->setStyleSheet(btnStyle);
    btnFontColor->setStyleSheet(btnStyle);
    btnBgColor->setStyleSheet(btnStyle);
    btnDescOrder->setStyleSheet(btnStyle);
    btnAscOrder->setStyleSheet(btnStyle);
    btnExport->setStyleSheet(btnStyle);
    btnUpload->setStyleSheet("QPushButton { background-color: blue; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px; }"
                             "QPushButton:hover { background-color: #0000CD; }");

    btnLayout->addWidget(btnAddRow);
    btnLayout->addWidget(btnDeleteColumn);
    btnLayout->addWidget(btnAddColumn);
    btnLayout->addWidget(btnFontColor);
    btnLayout->addWidget(btnBgColor);
    btnLayout->addWidget(btnDescOrder);
    btnLayout->addWidget(btnAscOrder);
    btnLayout->addWidget(btnExport);
    btnLayout->addWidget(btnUpload);
    qDebug() << "上传按钮已添加到布局，按钮是否可见:" << btnUpload->isVisible() << "，是否启用:" << btnUpload->isEnabled();
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);
    qDebug() << "按钮布局已添加到主布局";

    // 说明文本框
    QLabel* lblDesc = new QLabel("说明:");
    lblDesc->setStyleSheet("font-weight: bold; color: black;");
    mainLayout->addWidget(lblDesc);

    textDescription = new QTextEdit;
    textDescription->setPlaceholderText("在此处添加表格说明...");
    textDescription->setFixedHeight(60);
    textDescription->setStyleSheet("background-color: #ffa500; color: black; padding: 5px; border: 1px solid #888;");
    textDescription->setPlainText("说明:该表为期中成绩表。语文总分120、数学总分120");
    mainLayout->addWidget(textDescription);

    // 表格
    table = new QTableWidget(6, 8); // 初始6行，8列（学号、姓名、语文、数学、英语、总分 + 2个空白列）
    QStringList headers = { "学号", "姓名", "语文", "数学", "英语", "总分", "", "" };
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

    // 初始化表格数据
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem("");
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, col, item);
        }
    }

    // 添加示例数据
    if (table->item(0, 2)) table->item(0, 2)->setText("100");
    if (table->item(0, 3)) table->item(0, 3)->setText("89");
    if (table->item(1, 2)) table->item(1, 2)->setText("90");
    if (table->item(1, 3)) table->item(1, 3)->setText("78");
    if (table->item(2, 2)) table->item(2, 2)->setText("97");
    if (table->item(2, 3)) table->item(2, 3)->setText("80");
    if (table->item(3, 2)) table->item(3, 2)->setText("67");
    if (table->item(3, 3)) table->item(3, 3)->setText("97");

    mainLayout->addWidget(table);

    // 固定列索引（不能删除的列）
    fixedColumns = { 0, 1, 2, 3, 4, 5 }; // 学号、姓名、语文、数学、英语、总分
    nameColumnIndex = 1; // 姓名列索引

    // 连接信号和槽
    connect(btnAddRow, &QPushButton::clicked, this, &MidtermGradeDialog::onAddRow);
    connect(btnDeleteColumn, &QPushButton::clicked, this, &MidtermGradeDialog::onDeleteColumn);
    connect(btnAddColumn, &QPushButton::clicked, this, &MidtermGradeDialog::onAddColumn);
    connect(btnFontColor, &QPushButton::clicked, this, &MidtermGradeDialog::onFontColor);
    connect(btnBgColor, &QPushButton::clicked, this, &MidtermGradeDialog::onBgColor);
    connect(btnDescOrder, &QPushButton::clicked, this, &MidtermGradeDialog::onDescOrder);
    connect(btnAscOrder, &QPushButton::clicked, this, &MidtermGradeDialog::onAscOrder);
    connect(btnExport, &QPushButton::clicked, this, &MidtermGradeDialog::onExport);
    connect(btnUpload, &QPushButton::clicked, this, [this]() {
        qDebug() << "上传按钮被点击！";
        this->onUpload();
    });
    
    // 初始化网络管理器
    networkManager = new QNetworkAccessManager(this);

    // 单元格点击和悬浮事件
    connect(table, &QTableWidget::cellClicked, this, &MidtermGradeDialog::onCellClicked);
    connect(table, &QTableWidget::cellEntered, this, &MidtermGradeDialog::onCellEntered);
    table->setMouseTracking(true);

    // 右键菜单用于删除行
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QTableWidget::customContextMenuRequested, this, &MidtermGradeDialog::onTableContextMenu);
}

void MidtermGradeDialog::importData(const QStringList& headers, const QList<QStringList>& dataRows)
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
    int colId = -1, colName = -1, colChinese = -1, colMath = -1, colEnglish = -1, colTotal = -1;
    
    // 在表格中找到对应的列
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        
        if (headerText == "学号") colId = col;
        else if (headerText == "姓名") colName = col;
        else if (headerText == "语文") colChinese = col;
        else if (headerText == "数学") colMath = col;
        else if (headerText == "英语") colEnglish = col;
        else if (headerText == "总分") colTotal = col;
    }

    // 导入数据
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
        if (colChinese >= 0 && headerMap.contains("语文")) {
            int srcCol = headerMap["语文"];
            if (srcCol < rowData.size()) {
                table->item(row, colChinese)->setText(rowData[srcCol]);
            }
        }
        if (colMath >= 0 && headerMap.contains("数学")) {
            int srcCol = headerMap["数学"];
            if (srcCol < rowData.size()) {
                table->item(row, colMath)->setText(rowData[srcCol]);
            }
        }
        if (colEnglish >= 0 && headerMap.contains("英语")) {
            int srcCol = headerMap["英语"];
            if (srcCol < rowData.size()) {
                table->item(row, colEnglish)->setText(rowData[srcCol]);
            }
        }
        if (colTotal >= 0 && headerMap.contains("总分")) {
            int srcCol = headerMap["总分"];
            if (srcCol < rowData.size()) {
                table->item(row, colTotal)->setText(rowData[srcCol]);
            }
        }
    }
}

void MidtermGradeDialog::onAddRow()
{
    int currentRow = table->currentRow();
    if (currentRow < 0) {
        currentRow = table->rowCount() - 1;
    }
    int insertRow = currentRow + 1;
    table->insertRow(insertRow);

    // 初始化新行的所有单元格
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* item = new QTableWidgetItem("");
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(insertRow, col, item);
    }
}

void MidtermGradeDialog::onDeleteColumn()
{
    int currentCol = table->currentColumn();
    if (currentCol < 0) {
        QMessageBox::information(this, "提示", "请先选择要删除的列");
        return;
    }

    // 检查是否是固定列
    if (fixedColumns.contains(currentCol)) {
        QMessageBox::warning(this, "警告", "不能删除固定列（学号、姓名、语文、数学、英语、总分）");
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
        if (nameColumnIndex > currentCol) {
            nameColumnIndex--;
        }
    }
}

void MidtermGradeDialog::onAddColumn()
{
    bool ok;
    QString columnName = QInputDialog::getText(this, "添加列", "请输入列名:", 
                                               QLineEdit::Normal, "", &ok);
    if (!ok || columnName.isEmpty()) {
        return;
    }

    // 在姓名列后添加
    int insertCol = nameColumnIndex + 1;
    table->insertColumn(insertCol);
    table->setHorizontalHeaderItem(insertCol, new QTableWidgetItem(columnName));

    // 初始化新列的所有单元格
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* item = new QTableWidgetItem("");
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, insertCol, item);
    }

    // 更新固定列索引（插入列后，姓名列之后的固定列索引会变化）
    QSet<int> newFixedColumns;
    for (int col : fixedColumns) {
        if (col <= nameColumnIndex) {
            newFixedColumns.insert(col); // 姓名列及其之前的列索引不变
        } else {
            newFixedColumns.insert(col + 1); // 姓名列之后的列索引+1
        }
    }
    fixedColumns = newFixedColumns;
    // nameColumnIndex 不变，因为姓名列索引没有变化
}

void MidtermGradeDialog::onFontColor()
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

void MidtermGradeDialog::onBgColor()
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

void MidtermGradeDialog::onDescOrder()
{
    sortTable(false); // false表示倒序
}

void MidtermGradeDialog::onAscOrder()
{
    sortTable(true); // true表示正序
}

void MidtermGradeDialog::onExport()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出成绩表", "", "CSV文件 (*.csv);;所有文件 (*.*)");
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

void MidtermGradeDialog::onUpload()
{
    qDebug() << "onUpload() 方法被调用！";
    
    // 检查表格是否有数据
    if (table->rowCount() == 0) {
        qDebug() << "表格中没有数据，无法上传！";
        QMessageBox::warning(this, "提示", "表格中没有数据，无法上传！");
        return;
    }
    
    qDebug() << "表格有数据，行数:" << table->rowCount();

    // 使用类中的 m_classid
    QString classId = m_classid;
    if (classId.isEmpty()) {
        QMessageBox::warning(this, "错误", "班级ID为空，无法上传！");
        return;
    }

    // 考试名称默认为"期中考试"
    QString examName = "期中考试";

    // 根据当前日期计算学期
    QDate currentDate = QDate::currentDate();
    int year = currentDate.year();
    int month = currentDate.month();
    
    QString term;
    // 9月-1月是上学期（第一学期），2月-8月是下学期（第二学期）
    if (month >= 9 || month <= 1) {
        // 上学期：如果月份是9-12月，学年是 year-year+1-1；如果是1月，学年是 year-1-year-1
        if (month >= 9) {
            term = QString("%1-%2-1").arg(year).arg(year + 1);
        } else {
            term = QString("%1-%2-1").arg(year - 1).arg(year);
        }
    } else {
        // 下学期：2-8月，学年是 year-1-year-2
        term = QString("%1-%2-2").arg(year - 1).arg(year);
    }
    
    qDebug() << "当前日期:" << currentDate.toString("yyyy-MM-dd") << "，计算出的学期:" << term;

    // 备注（可选）
    QString remark = QString("%1年%2学期期中考试")
                     .arg(currentDate.year())
                     .arg(month >= 9 || month <= 1 ? "第一" : "第二");

    // 从表格中读取数据
    QJsonArray scoresArray;
    
    // 获取列索引
    int colId = -1, colName = -1, colChinese = -1, colMath = -1, colEnglish = -1;
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "学号") colId = col;
        else if (headerText == "姓名") colName = col;
        else if (headerText == "语文") colChinese = col;
        else if (headerText == "数学") colMath = col;
        else if (headerText == "英语") colEnglish = col;
    }

    if (colId < 0 || colName < 0) {
        QMessageBox::warning(this, "错误", "表格中缺少必要列：学号、姓名！");
        return;
    }

    // 读取每一行数据
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* idItem = table->item(row, colId);
        QTableWidgetItem* nameItem = table->item(row, colName);
        
        if (!idItem || !nameItem) continue;
        
        QString studentId = idItem->text().trimmed();
        QString studentName = nameItem->text().trimmed();
        
        // 至少要有学号或姓名
        if (studentId.isEmpty() && studentName.isEmpty()) {
            continue; // 跳过空行
        }

        QJsonObject scoreObj;
        if (!studentId.isEmpty()) {
            scoreObj["student_id"] = studentId;
        }
        if (!studentName.isEmpty()) {
            scoreObj["student_name"] = studentName;
        }

        // 读取成绩
        if (colChinese >= 0) {
            QTableWidgetItem* item = table->item(row, colChinese);
            if (item && !item->text().trimmed().isEmpty()) {
                bool ok;
                int score = item->text().toInt(&ok);
                if (ok) {
                    scoreObj["chinese"] = score;
                }
            }
        }
        if (colMath >= 0) {
            QTableWidgetItem* item = table->item(row, colMath);
            if (item && !item->text().trimmed().isEmpty()) {
                bool ok;
                int score = item->text().toInt(&ok);
                if (ok) {
                    scoreObj["math"] = score;
                }
            }
        }
        if (colEnglish >= 0) {
            QTableWidgetItem* item = table->item(row, colEnglish);
            if (item && !item->text().trimmed().isEmpty()) {
                bool ok;
                int score = item->text().toInt(&ok);
                if (ok) {
                    scoreObj["english"] = score;
                }
            }
        }

        scoresArray.append(scoreObj);
    }

    if (scoresArray.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有有效的学生数据！");
        return;
    }

    // 构造请求 JSON
    QJsonObject requestObj;
    requestObj["class_id"] = classId;
    requestObj["exam_name"] = examName;
    requestObj["term"] = term;
    if (!remark.isEmpty()) {
        requestObj["remark"] = remark;
    }
    requestObj["scores"] = scoresArray;

    QJsonDocument doc(requestObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // 发送 POST 请求
    QString url = "http://47.100.126.194:5000/student-scores/save";
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager->post(request, jsonData);

    // 显示上传中提示
    QMessageBox* progressMsg = new QMessageBox(this);
    progressMsg->setWindowTitle("上传中");
    progressMsg->setText("正在上传成绩数据到服务器...");
    progressMsg->setStandardButtons(QMessageBox::NoButton);
    progressMsg->show();

    // 处理响应
    connect(reply, &QNetworkReply::finished, this, [=]() {
        progressMsg->close();
        progressMsg->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
            
            if (responseDoc.isObject()) {
                QJsonObject responseObj = responseDoc.object();
                int code = responseObj["code"].toInt();
                
                if (code == 200) {
                    QJsonObject dataObj = responseObj["data"].toObject();
                    QString message = dataObj["message"].toString();
                    int insertedCount = dataObj["inserted_count"].toInt();
                    
                    QMessageBox::information(this, "上传成功", 
                        QString("上传成功！\n\n%1\n共插入 %2 条记录").arg(message).arg(insertedCount));
                    
                    // 上传成功后自动打开排座窗口
                    openSeatingArrangementDialog();
                } else {
                    QString errorMsg = responseObj["message"].toString();
                    QMessageBox::warning(this, "上传失败", 
                        QString("服务器返回错误：\n%1").arg(errorMsg));
                }
            } else {
                QMessageBox::information(this, "上传成功", "数据已成功上传到服务器！");
            }
        } else {
            QString errorString = reply->errorString();
            QByteArray errorData = reply->readAll();
            QString errorMsg = QString::fromUtf8(errorData);
            
            QMessageBox::critical(this, "上传失败", 
                QString("网络错误：%1\n%2").arg(errorString).arg(errorMsg));
        }
        
        reply->deleteLater();
    });
}

void MidtermGradeDialog::onCellClicked(int row, int column)
{
    showCellComment(row, column);
}

void MidtermGradeDialog::onCellEntered(int row, int column)
{
    // 鼠标悬浮时显示注释提示
    QTableWidgetItem* item = table->item(row, column);
    if (item) {
        QString comment = item->data(Qt::UserRole).toString();
        if (!comment.isEmpty()) {
            QToolTip::showText(QCursor::pos(), comment, table);
        }
    }
}

void MidtermGradeDialog::onTableContextMenu(const QPoint& pos)
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
        int ret = QMessageBox::question(this, "确认", "确定要删除这一行吗？", 
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            table->removeRow(row);
        }
    }
}

void MidtermGradeDialog::sortTable(bool ascending)
{
    int sortColumn = table->currentColumn();
    if (sortColumn < 0) {
        QMessageBox::information(this, "提示", "请先选择要排序的列");
        return;
    }

    // 获取所有行数据
    QList<QList<QTableWidgetItem*>> rowsData;
    for (int row = 0; row < table->rowCount(); ++row) {
        QList<QTableWidgetItem*> rowData;
        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (item) {
                // 创建新的item副本
                QTableWidgetItem* newItem = item->clone();
                rowData.append(newItem);
            } else {
                rowData.append(new QTableWidgetItem(""));
            }
        }
        rowsData.append(rowData);
    }

    // 按指定列排序
    std::sort(rowsData.begin(), rowsData.end(), [sortColumn, ascending](const QList<QTableWidgetItem*>& a, const QList<QTableWidgetItem*>& b) {
        if (sortColumn >= a.size() || sortColumn >= b.size()) return false;
        QString textA = a[sortColumn] ? a[sortColumn]->text() : "";
        QString textB = b[sortColumn] ? b[sortColumn]->text() : "";
        
        // 尝试转换为数字
        bool okA, okB;
        double numA = textA.toDouble(&okA);
        double numB = textB.toDouble(&okB);
        
        if (okA && okB) {
            return ascending ? (numA < numB) : (numA > numB);
        } else {
            return ascending ? (textA < textB) : (textA > textB);
        }
    });

    // 重新设置表格数据
    table->setRowCount(rowsData.size());
    for (int row = 0; row < rowsData.size(); ++row) {
        for (int col = 0; col < rowsData[row].size(); ++col) {
            table->setItem(row, col, rowsData[row][col]);
        }
    }
}

void MidtermGradeDialog::openSeatingArrangementDialog()
{
    // 从表格中读取学生数据
    QList<StudentInfo> students;
    
    // 获取列索引
    int colId = -1, colName = -1, colTotal = -1;
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* headerItem = table->horizontalHeaderItem(col);
        if (!headerItem) continue;
        QString headerText = headerItem->text();
        if (headerText == "学号") colId = col;
        else if (headerText == "姓名") colName = col;
        else if (headerText == "总分") colTotal = col;
    }
    
    if (colName < 0) {
        QMessageBox::warning(this, "错误", "表格中缺少姓名列！");
        return;
    }
    
    // 读取每一行数据
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* nameItem = table->item(row, colName);
        if (!nameItem || nameItem->text().trimmed().isEmpty()) {
            continue; // 跳过空行
        }
        
        StudentInfo student;
        student.name = nameItem->text().trimmed();
        if (colId >= 0) {
            QTableWidgetItem* idItem = table->item(row, colId);
            if (idItem) {
                student.id = idItem->text().trimmed();
            }
        }
        
        // 获取总分作为排序依据
        if (colTotal >= 0) {
            QTableWidgetItem* totalItem = table->item(row, colTotal);
            if (totalItem && !totalItem->text().trimmed().isEmpty()) {
                bool ok;
                student.score = totalItem->text().toDouble(&ok);
                if (!ok) student.score = 0;
            } else {
                student.score = 0;
            }
        } else {
            student.score = 0;
        }
        
        student.originalIndex = students.size();
        students.append(student);
    }
    
    if (students.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有有效的学生数据！");
        return;
    }
    
    // 找到所有打开的 ScheduleDialog 实例
    QList<ScheduleDialog*> scheduleDialogs;
    QWidgetList widgets = QApplication::allWidgets();
    for (QWidget* widget : widgets) {
        ScheduleDialog* scheduleDlg = qobject_cast<ScheduleDialog*>(widget);
        if (scheduleDlg && !scheduleDlg->isHidden()) {
            scheduleDialogs.append(scheduleDlg);
            qDebug() << "找到打开的 ScheduleDialog:" << scheduleDlg;
        }
    }
    
    qDebug() << "共找到" << scheduleDialogs.size() << "个打开的 ScheduleDialog 实例";
    
    if (scheduleDialogs.isEmpty()) {
        QMessageBox::information(this, "提示", "请先打开一个班级群聊窗口！");
        return;
    }
    
    // 显示排座方式选择对话框
    ArrangeSeatDialog* arrangeDialog = new ArrangeSeatDialog(this);
    if (arrangeDialog->exec() == QDialog::Accepted) {
        QString method = arrangeDialog->getSelectedMethod();
        qDebug() << "用户选择的排座方式:" << method;
        qDebug() << "学生数据数量:" << students.size();
        
        // 对所有打开的 ScheduleDialog 执行排座
        for (ScheduleDialog* scheduleDlg : scheduleDialogs) {
            qDebug() << "正在为 ScheduleDialog 执行排座...";
            scheduleDlg->arrangeSeats(students, method);
        }
        
        QMessageBox::information(this, "排座完成", 
            QString("已为 %1 个班级群聊窗口完成排座！\n排座方式：%2").arg(scheduleDialogs.size()).arg(method), 
            QMessageBox::Ok);
    }
    
    arrangeDialog->deleteLater();
}

void MidtermGradeDialog::showCellComment(int row, int column)
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
            item->setBackground(QBrush(QColor(255, 255, 200))); // 浅黄色
        } else {
            item->setBackground(QBrush()); // 恢复默认
        }
    }
}

