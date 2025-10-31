#pragma once
#pragma once
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include "CourseTableWidget.h"

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

        layout->addWidget(btnImport, 0, Qt::AlignLeft);
        layout->addWidget(exportBtn, 0, Qt::AlignLeft);

        m_table = new CourseTableWidget(this);
        m_table->setStyleSheet(
            "QHeaderView::section{background-color:#2f3240;color:white;font-weight:bold;}"
            "QTableWidget::item{border:1px solid lightgray;}"
            "QTableWidget::item:selected{background-color:#3399ff;color:white;}"
        );
        layout->addWidget(m_table, 1);

        connect(btnImport, &QPushButton::clicked, this, &CourseDialog::onImport);
        connect(exportBtn, &QPushButton::clicked, this, &CourseDialog::onExportClicked);
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

        // 写列标题
        QStringList headers;
        for (int col = 0; col < m_table->columnCount(); ++col) {
            headers << m_table->horizontalHeaderItem(col)->text();
        }
        out << headers.join(",") << "\n";

        // 写每行数据
        for (int row = 0; row < m_table->rowCount(); ++row) {
            QStringList rowData;
            for (int col = 0; col < m_table->columnCount(); ++col) {
                QTableWidgetItem* item = m_table->item(row, col);
                rowData << (item ? item->text() : "");
            }
            out << rowData.join(",") << "\n";
        }

        file.close();
    }

private:
    CourseTableWidget* m_table{};
};
