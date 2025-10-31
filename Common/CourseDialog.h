#pragma once
#pragma once
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "CourseTableWidget.h"
#include "TAHttpHandler.h"

class CourseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CourseDialog(QWidget* parent = nullptr)
        : QDialog(parent)
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
    }

private slots:
    void onImport() {
        QString file = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择课程表文件"), QString(),
            QStringLiteral("CSV 文件 (*.csv);;所有文件 (*.*)")
        );
        if (file.isEmpty())
            return;
        // TODO: 这里读取文件并填充表格，可按需实现
        QMessageBox::information(this, QStringLiteral("提示"),
            QStringLiteral("已选择文件:\n") + file);
    }

    void onExportClicked()
    {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "导出课表",
            "",
            "CSV 文件 (*.csv);;所有文件 (*.*)"
        );
        if (fileName.isEmpty())
            return;

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;

        QTextStream out(&file);

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
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("课程表导出完成"));
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
        payload["class_id"] = "000011001"; // 可按需要替换（原 group_id 改为 class_id）
        payload["term"] = "2025-2026-1";   // 可按需要替换
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
};
