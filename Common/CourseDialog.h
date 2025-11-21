#pragma once
#pragma execution_character_set("utf-8")
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>
#include <QDate>
#include <QRegExp>
#include <QUrl>
#include <QUrlQuery>
#include "CourseTableWidget.h"
#include "TAHttpHandler.h"
#include "QXlsx/header/xlsxdocument.h"
#include "QXlsx/header/xlsxworksheet.h"
#include "QXlsx/header/xlsxcell.h"
#include "QXlsx/header/xlsxglobal.h"
QT_BEGIN_NAMESPACE_XLSX
QT_END_NAMESPACE_XLSX

class CourseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CourseDialog(QWidget* parent = nullptr)
        : QDialog(parent), m_unique_group_id(""), m_classid("")
    {
        setWindowTitle(QStringLiteral("课程表"));
        resize(850, 650);
        setStyleSheet("background-color:#f5d6c6;"); // 类似图片里的浅粉底色

        auto* layout = new QVBoxLayout(this);
        auto* btnImport = new QPushButton(QStringLiteral("导入"));
        btnImport->setFixedSize(60, 28);
        btnImport->setStyleSheet(
            "QPushButton{background-color:#608bd0;color:white;border-radius:4px;}"
            "QPushButton:hover{background-color:#4f78be;}"
        );

        QPushButton* exportBtn = new QPushButton("导出", this);
        exportBtn->setFixedSize(60, 28);
        exportBtn->setStyleSheet(
            "QPushButton{background-color:#608bd0;color:white;border-radius:4px;}"
            "QPushButton:hover{background-color:#4f78be;}"
        );

        QPushButton* sendBtn = new QPushButton("发送", this);
        sendBtn->setFixedSize(60, 28);
        sendBtn->setStyleSheet(
            "QPushButton{background-color:#608bd0;color:white;border-radius:4px;}"
            "QPushButton:hover{background-color:#4f78be;}"
        );

        layout->addWidget(btnImport, 0, Qt::AlignLeft);
        layout->addWidget(exportBtn, 0, Qt::AlignLeft);
        layout->addWidget(sendBtn, 0, Qt::AlignLeft);

        m_table = new CourseTableWidget(this);
        m_table->setStyleSheet(
            "QHeaderView::section{background-color:#2f3240;color:white;font-weight:bold;}"
            "QTableWidget::item{border:1px solid lightgray;}"
            "QTableWidget::item:selected{background-color:#3399ff;color:white;}"
        );
        layout->addWidget(m_table, 1);

        connect(btnImport, &QPushButton::clicked, this, &CourseDialog::onImport);
        connect(exportBtn, &QPushButton::clicked, this, &CourseDialog::onExportClicked);
        connect(sendBtn, &QPushButton::clicked, this, &CourseDialog::onSendToServer);

        // 初始化 HTTP 成员并绑定一次回调
        m_httpHandler = new TAHttpHandler(this);
        if (m_httpHandler)
        {
            connect(m_httpHandler, &TAHttpHandler::success, this, [this](const QString& resp){
                QJsonParseError err;
                QJsonDocument jd = QJsonDocument::fromJson(resp.toUtf8(), &err);
                if (err.error == QJsonParseError::NoError && jd.isObject()) {
                    QJsonObject obj = jd.object();
                    const int code = obj.value("code").toInt(-1);
                    const QString msg = obj.value("message").toString(QStringLiteral("保存成功"));
                    if (code == 200) {
                        QMessageBox::information(this, QStringLiteral("提示"), msg);
                    } else {
                        QMessageBox::warning(this, QStringLiteral("提示"), msg);
                    }
                } else {
                    QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("课程表已发送到服务器"));
                }
            });
            connect(m_httpHandler, &TAHttpHandler::failed, this, [this](const QString& err){
                QMessageBox::critical(this, QStringLiteral("错误"), QStringLiteral("发送失败：") + err);
            });
        }

        m_fetchHandler = new TAHttpHandler(this);
        if (m_fetchHandler)
        {
            connect(m_fetchHandler, &TAHttpHandler::success, this, [this](const QString& resp){
                handleScheduleResponse(resp);
            });
            connect(m_fetchHandler, &TAHttpHandler::failed, this, [this](const QString& err){
                QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("获取课程表失败：") + err);
            });
        }
    }

    // 设置群组ID
    void setGroupId(const QString& groupId) {
        m_unique_group_id = groupId;
    }

    // 设置班级ID
    void setClassId(const QString& classId) {
        m_classid = classId;
        fetchCourseSchedule();
    }

private slots:
    void onImport() {
        QString file = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择课程表文件"), QString(),
            QStringLiteral("Excel 文件 (*.xlsx);;CSV 文件 (*.csv);;所有文件 (*.*)")
        );
        if (file.isEmpty())
            return;

        // 判断文件类型
        if (file.endsWith(".xlsx", Qt::CaseInsensitive)) {
            importFromExcel(file);
        } else if (file.endsWith(".csv", Qt::CaseInsensitive)) {
            importFromCsv(file);
        } else {
            QMessageBox::warning(this, QStringLiteral("提示"), 
                QStringLiteral("不支持的文件格式，请选择 .xlsx 或 .csv 文件"));
        }
    }

private:
    void importFromExcel(const QString& filePath) {
        using namespace QXlsx;
        
        Document xlsx(filePath, this);
        if (!xlsx.isLoadPackage()) {
            QMessageBox::critical(this, QStringLiteral("错误"), 
                QStringLiteral("无法打开 Excel 文件：") + filePath);
            return;
        }

        // 获取第一个工作表
        QStringList sheetNames = xlsx.sheetNames();
        if (sheetNames.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("提示"), 
                QStringLiteral("Excel 文件中没有工作表"));
            return;
        }

        // 读取数据范围
        CellRange range = xlsx.dimension();
        if (!range.isValid()) {
            QMessageBox::warning(this, QStringLiteral("提示"), 
                QStringLiteral("Excel 文件为空或格式不正确"));
            return;
        }

        // 智能识别 Excel 格式：
        // 1. 检测第一行是否包含星期（周一、周二等），如果有则跳过第一行
        // 2. 检测第一列是否包含时间，如果有则跳过第一列

        int startRow = 1;  // Excel 行号从 1 开始
        int startCol = 1;  // Excel 列号从 1 开始

        // 检测第一行是否包含星期
        bool firstRowIsWeekday = false;
        QStringList weekdays = { QStringLiteral("周一"), QStringLiteral("周二"), QStringLiteral("周三"), 
                                 QStringLiteral("周四"), QStringLiteral("周五"), QStringLiteral("周六"), 
                                 QStringLiteral("周日"), QStringLiteral("星期一"), QStringLiteral("星期二"),
                                 QStringLiteral("星期三"), QStringLiteral("星期四"), QStringLiteral("星期五"),
                                 QStringLiteral("星期六"), QStringLiteral("星期日") };
        
        // 检查第一行的前几列是否包含星期
        for (int col = 1; col <= qMin(7, range.lastColumn()); ++col) {
            QVariant cellValue = xlsx.read(1, col);
            QString cellText = cellValue.toString().trimmed();
            for (const QString& weekday : weekdays) {
                if (cellText.contains(weekday)) {
                    firstRowIsWeekday = true;
                    break;
                }
            }
            if (firstRowIsWeekday) break;
        }
        
        if (firstRowIsWeekday) {
            startRow = 2;  // 跳过第一行
        }

        // 检测第一列是否包含时间
        bool firstColIsTime = false;
        // 时间格式：如 "6:00", "8:10", "14:10" 等
        QRegExp timePattern("^\\d{1,2}:\\d{2}$");
        
        // 检查第一列的前几行是否包含时间（从第一行开始检查，因为可能第一行第一列就是时间）
        int checkStartRow = firstRowIsWeekday ? 2 : 1;
        for (int row = checkStartRow; row <= qMin(checkStartRow + 10, range.lastRow()); ++row) {
            QVariant cellValue = xlsx.read(row, 1);
            QString cellText = cellValue.toString().trimmed();
            if (timePattern.exactMatch(cellText)) {
                firstColIsTime = true;
                break;
            }
        }
        
        if (firstColIsTime) {
            startCol = 2;  // 跳过第一列
        }

        // 清空表格
        for (int row = 0; row < m_table->rowCount(); ++row) {
            for (int col = 0; col < m_table->columnCount(); ++col) {
                m_table->setCourse(row, col, "", false);
            }
        }

        // 读取数据并填充表格
        int excelRow = startRow;
        int tableRow = 0;
        int maxRows = qMin(range.lastRow() - startRow + 1, m_table->rowCount());

        for (int i = 0; i < maxRows && tableRow < m_table->rowCount(); ++i, ++excelRow, ++tableRow) {
            int excelCol = startCol;
            int tableCol = 0;

            int maxCols = qMin(range.lastColumn() - startCol + 1, m_table->columnCount());
            for (int j = 0; j < maxCols && tableCol < m_table->columnCount(); ++j, ++excelCol, ++tableCol) {
                QVariant cellValue = xlsx.read(excelRow, excelCol);
                QString cellText = cellValue.toString().trimmed();

                if (!cellText.isEmpty()) {
                    // 检查单元格格式，判断是否为高亮（这里简单判断，可以根据实际需求调整）
                    Cell* cell = xlsx.cellAt(excelRow, excelCol);
                    bool isHighlight = false;
                    if (cell) {
                        Format format = cell->format();
                        QColor bgColor = format.patternBackgroundColor();
                        // 如果背景色不是白色或默认色，则认为是高亮
                        if (bgColor.isValid() && bgColor != QColor(Qt::white) && bgColor != QColor()) {
                            isHighlight = true;
                        }
                    }
                    m_table->setCourse(tableRow, tableCol, cellText, isHighlight);
                }
            }
        }

        QMessageBox::information(this, QStringLiteral("提示"), 
            QStringLiteral("Excel 文件导入成功"));
    }

    void importFromCsv(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, QStringLiteral("错误"), 
                QStringLiteral("无法打开 CSV 文件：") + filePath);
            return;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");

        // 清空表格
        for (int row = 0; row < m_table->rowCount(); ++row) {
            for (int col = 0; col < m_table->columnCount(); ++col) {
                m_table->setCourse(row, col, "", false);
            }
        }

        // 读取 CSV 数据
        int row = 0;
        bool skipFirstRow = true; // 跳过第一行（表头）

        while (!in.atEnd() && row < m_table->rowCount()) {
            QString line = in.readLine();
            if (skipFirstRow) {
                skipFirstRow = false;
                continue;
            }

            // 改进的 CSV 解析，处理包含逗号的字段（用引号括起来的）
            QStringList values;
            QString currentValue;
            bool inQuotes = false;
            
            for (int i = 0; i < line.length(); ++i) {
                QChar ch = line[i];
                if (ch == '"') {
                    inQuotes = !inQuotes;
                } else if (ch == ',' && !inQuotes) {
                    values.append(currentValue.trimmed());
                    currentValue.clear();
                } else {
                    currentValue += ch;
                }
            }
            // 添加最后一个值
            if (!currentValue.isEmpty() || line.endsWith(',')) {
                values.append(currentValue.trimmed());
            }
            
            int col = 0;
            // 跳过第一列（如果是时间列）
            int startCol = (skipFirstRow && values.size() > 0 && values[0].isEmpty()) ? 1 : 0;
            for (int i = startCol; i < values.size() && col < m_table->columnCount(); ++i) {
                QString value = values[i];
                // 移除引号（如果存在）
                if (value.startsWith("\"") && value.endsWith("\"") && value.length() >= 2) {
                    value = value.mid(1, value.length() - 2);
                }
                // 处理转义的引号
                value.replace("\"\"", "\"");
                if (!value.isEmpty()) {
                    m_table->setCourse(row, col, value, false);
                }
                ++col;
            }
            ++row;
        }

        file.close();
        QMessageBox::information(this, QStringLiteral("提示"), 
            QStringLiteral("CSV 文件导入成功"));
    }

    void onExportClicked()
    {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("导出课表"),
            "",
            QStringLiteral("Excel 文件 (*.xlsx);;CSV 文件 (*.csv);;所有文件 (*.*)")
        );
        if (fileName.isEmpty())
            return;

        // 判断文件类型
        if (fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
            exportToExcel(fileName);
        } else {
            exportToCsv(fileName);
        }
    }

private:
    void exportToExcel(const QString& filePath) {
        using namespace QXlsx;
        
        Document xlsx(this);
        
        // 写入表头（第一行第一列为空，第一行从第二列开始是星期）
        int row = 1;
        int col = 1;
        
        // 第一行第一列留空
        xlsx.write(row, col, "");
        col++;
        
        // 写入星期表头
        for (int c = 0; c < m_table->columnCount(); ++c) {
            QTableWidgetItem* headerItem = m_table->horizontalHeaderItem(c);
            QString headerText = headerItem ? headerItem->text() : "";
            xlsx.write(row, col, headerText);
            col++;
        }
        
        // 写入数据行（第一列是时间，从第二列开始是课程）
        row = 2;
        for (int r = 0; r < m_table->rowCount(); ++r) {
            col = 1;
            
            // 第一列写入时间
            QTableWidgetItem* vHeaderItem = m_table->verticalHeaderItem(r);
            QString timeText = vHeaderItem ? vHeaderItem->text() : "";
            xlsx.write(row, col, timeText);
            col++;
            
            // 写入课程数据
            for (int c = 0; c < m_table->columnCount(); ++c) {
                QTableWidgetItem* item = m_table->item(r, c);
                QString courseText = item ? item->text() : "";
                
                if (!courseText.isEmpty()) {
                    // 检查是否为高亮课程
                    bool isHighlight = item && item->background().color() != QColor(Qt::white);
                    
                    Format format;
                    if (isHighlight) {
                        // 设置高亮格式（蓝色背景，白色文字）
                        format.setPatternBackgroundColor(QColor(51, 153, 255));
                        format.setFontColor(Qt::white);
                    }
                    
                    xlsx.write(row, col, courseText, format);
                }
                col++;
            }
            row++;
        }
        
        // 保存文件
        if (xlsx.saveAs(filePath)) {
            QMessageBox::information(this, QStringLiteral("提示"), 
                QStringLiteral("Excel 文件导出成功"));
        } else {
            QMessageBox::critical(this, QStringLiteral("错误"), 
                QStringLiteral("Excel 文件导出失败"));
        }
    }

    void exportToCsv(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QTextStream out(&file);
        out.setCodec("UTF-8");

        // 写列标题（首列为空以对齐行标题）
        QStringList headers;
        headers << ""; // 行标题占位
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem* headerItem = m_table->horizontalHeaderItem(col);
            headers << (headerItem ? headerItem->text() : "");
        }
        out << headers.join(",") << "\n";

        // 写每行数据（首列写入行标题）
        for (int row = 0; row < m_table->rowCount(); ++row) {
            QStringList rowData;
            QTableWidgetItem* vHeaderItem = m_table->verticalHeaderItem(row);
            rowData << (vHeaderItem ? vHeaderItem->text() : "");
            for (int col = 0; col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = m_table->item(row, col);
                rowData << (item ? item->text() : "");
            }
            out << rowData.join(",") << "\n";
        }

        file.close();
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("CSV 文件导出完成"));
    }

    void onSendToServer()
    {
        // 1) 收集表头 days 与 times
        QJsonArray days;
        for (int col = 0; col < m_table->columnCount(); ++col) {
            auto* hi = m_table->horizontalHeaderItem(col);
            days.append(hi ? hi->text() : "");
        }

        QJsonArray times;
        for (int row = 0; row < m_table->rowCount(); ++row) {
            auto* vi = m_table->verticalHeaderItem(row);
            times.append(vi ? vi->text() : "");
        }

        // 2) 收集单元格
        QJsonArray cells;
        for (int row = 0; row < m_table->rowCount(); ++row) {
            for (int col = 0; col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = m_table->item(row, col);
                const QString name = item ? item->text() : "";
                if (!name.isEmpty()) {
                    QJsonObject cell;
                    cell["row_index"] = row;
                    cell["col_index"] = col;
                    cell["course_name"] = name;
                    // 简单以背景是否非白作为高亮判断（与 setCourse 的用法保持一致可改造）
                    const bool isHighlight = item && item->background().color() != QColor(Qt::white);
                    cell["is_highlight"] = isHighlight ? 1 : 0;
                    cells.append(cell);
                }
            }
        }

        // 3) 组装 JSON（契合后端 /course-schedule/save 接口）
        QJsonObject payload;
        
        // 使用传入的班级id，如果没有则使用默认值
        QString classId = m_classid.isEmpty() ? "000011001" : m_classid;
        
        payload["class_id"] = classId;
        payload["group_id"] = m_unique_group_id; // 添加群组id
        payload["term"] = currentTermString();   // 当前最新学期
        payload["days"] = days;
        payload["times"] = times;
        payload["cells"] = cells;
        payload["remark"] = ""; // 备注可按需填写

        QJsonDocument doc(payload);
        const QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 4) 发送到服务器（使用提供的保存接口）
        const QString url = QStringLiteral("http://47.100.126.194:5000/course-schedule/save");
        if (m_httpHandler)
        {
            m_httpHandler->post(url, jsonData);
        }
    }

private:
    CourseTableWidget* m_table{};
    TAHttpHandler* m_httpHandler{};
    TAHttpHandler* m_fetchHandler{};
    QString m_unique_group_id; // 群组ID
    QString m_classid; // 班级ID
    
    QString currentTermString() const
    {
        QDate today = QDate::currentDate();
        int year = today.year();
        int month = today.month();

        int startYear = year;
        int endYear = year + 1;
        int termIndex = 1;

        if (month >= 9) {
            startYear = year;
            endYear = year + 1;
            termIndex = 1;
        } else if (month >= 3 && month <= 8) {
            startYear = year - 1;
            endYear = year;
            termIndex = 2;
        } else {
            // 1-2月，仍属于上一学年的第一学期
            startYear = year - 1;
            endYear = year;
            termIndex = 1;
        }

        return QString("%1-%2-%3")
            .arg(startYear)
            .arg(endYear)
            .arg(termIndex);
    }

    void fetchCourseSchedule()
    {
        if (!m_fetchHandler || m_classid.isEmpty())
            return;

        const QString term = currentTermString();
        QUrl url(QStringLiteral("http://47.100.126.194:5000/course-schedule"));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("class_id"), m_classid);
        query.addQueryItem(QStringLiteral("term"), term);
        url.setQuery(query);
        m_fetchHandler->get(url.toString());
    }

    void handleScheduleResponse(const QString& resp)
    {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        {
            QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("课程表数据解析失败"));
            return;
        }

        QJsonObject obj = doc.object();
        const int code = obj.value(QStringLiteral("code")).toInt(-1);
        const QString message = obj.value(QStringLiteral("message")).toString(QStringLiteral("查询失败"));
        if (code != 200)
        {
            QMessageBox::information(this, QStringLiteral("提示"), message);
            return;
        }

        QJsonObject dataObj = obj.value(QStringLiteral("data")).toObject();
        QJsonObject scheduleObj = dataObj.value(QStringLiteral("schedule")).toObject();
        QJsonArray daysArray = scheduleObj.value(QStringLiteral("days")).toArray();
        QJsonArray timesArray = scheduleObj.value(QStringLiteral("times")).toArray();
        QJsonArray cellsArray = dataObj.value(QStringLiteral("cells")).toArray();

        QStringList days = jsonArrayToStringList(daysArray);
        QStringList times = jsonArrayToStringList(timesArray);
        applySchedule(days, times, cellsArray);
    }

    QStringList jsonArrayToStringList(const QJsonArray& arr) const
    {
        QStringList list;
        for (const QJsonValue& value : arr)
        {
            list << value.toString();
        }
        return list;
    }

    void applySchedule(const QStringList& days, const QStringList& times, const QJsonArray& cells)
    {
        if (!days.isEmpty())
        {
            m_table->setColumnCount(days.size());
            m_table->setHorizontalHeaderLabels(days);
        }

        if (!times.isEmpty())
        {
            m_table->setRowCount(times.size());
            m_table->setVerticalHeaderLabels(times);
        }

        for (int r = 0; r < m_table->rowCount(); ++r)
        {
            for (int c = 0; c < m_table->columnCount(); ++c)
            {
                m_table->setCourse(r, c, QString(), false);
            }
        }

        bool filteredFirstColumn = false;
        for (const QJsonValue& value : cells)
        {
            if (!value.isObject())
                continue;

            QJsonObject cellObj = value.toObject();
            int colIndex = cellObj.value(QStringLiteral("col_index")).toInt(-1);
            QString courseName = cellObj.value(QStringLiteral("course_name")).toString();
            if (colIndex == 0 && looksLikeTimeText(courseName))
            {
                filteredFirstColumn = true;
                break;
            }
        }

        for (const QJsonValue& value : cells)
        {
            if (!value.isObject())
                continue;

            QJsonObject cellObj = value.toObject();
            int rowIndex = cellObj.value(QStringLiteral("row_index")).toInt(-1);
            int colIndex = cellObj.value(QStringLiteral("col_index")).toInt(-1);
            QString courseName = cellObj.value(QStringLiteral("course_name")).toString();
            bool isHighlight = cellObj.value(QStringLiteral("is_highlight")).toInt(0) != 0;

            if (rowIndex < 0 || colIndex < 0)
                continue;
            if (rowIndex >= m_table->rowCount())
                continue;
            if (courseName.isEmpty())
                continue;
            if (colIndex == 0 && looksLikeTimeText(courseName))
                continue;

            int targetCol = colIndex;
            if (filteredFirstColumn)
            {
                targetCol = colIndex - 1;
                if (targetCol < 0)
                    continue;
            }

            if (targetCol >= m_table->columnCount())
                continue;

            m_table->setCourse(rowIndex, targetCol, courseName, isHighlight);
        }
    }

    bool looksLikeTimeText(const QString& text) const
    {
        // 支持格式：HH:MM、HH:MM:SS、HH:MM:SS.mmm
        static QRegExp preciseTimePattern(QStringLiteral("^\\d{1,2}:\\d{2}(?::\\d{2}(?:\\.\\d{1,3})?)?$"));
        if (preciseTimePattern.exactMatch(text))
            return true;

        // 兼容形如 “08:00-08:45” 的区间
        QRegExp rangePattern(QStringLiteral("^(\\d{1,2}:\\d{2})\\s*-\\s*(\\d{1,2}:\\d{2})$"));
        if (rangePattern.exactMatch(text))
            return true;

        return false;
    }
};
