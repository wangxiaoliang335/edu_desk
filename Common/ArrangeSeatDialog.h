#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QFrame>
#include <QBrush>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QCoreApplication>
#include <QMap>
#include <QDebug>
#include <QShowEvent>
#include <QFile>
#include <QTextStream>
#include "QXlsx/header/xlsxdocument.h"
#include "CommonInfo.h"

class ArrangeSeatDialog : public QDialog
{
    Q_OBJECT

public:
    ArrangeSeatDialog(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("排座");
        resize(400, 350);
        setStyleSheet("background-color: #e0f0ff; font-size:14px;");

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // 顶部：两个列表框并排
        QHBoxLayout* listLayout = new QHBoxLayout;
        listLayout->setSpacing(20);

        // 左侧下拉框：小组/不分组
        QFrame* leftFrame = new QFrame;
        leftFrame->setFrameShape(QFrame::StyledPanel);
        leftFrame->setStyleSheet("background-color: #e0f0ff; border: 2px solid #87ceeb;");
        QVBoxLayout* leftLayout = new QVBoxLayout(leftFrame);
        leftLayout->setContentsMargins(5, 5, 5, 5);

        leftComboBox = new QComboBox;
        leftComboBox->setEditable(false); // 设置为不可编辑，确保显示选中的项
        leftComboBox->setStyleSheet(
            "QComboBox { background-color: white; border: 1px solid #ccc; padding: 8px; min-width: 120px; color: black; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: white; selection-background-color: #ff8c00; color: black; }"
            "QComboBox QAbstractItemView::item { padding: 8px; color: black; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #ff8c00; color: white; }"
        );

        // 添加左侧选项
        leftComboBox->addItem("小组");
        leftComboBox->addItem("不分组");

        // 默认选择"不分组"（索引1）
        leftComboBox->setCurrentIndex(1);
        leftComboBox->update(); // 强制更新显示

        leftLayout->addWidget(leftComboBox);
        listLayout->addWidget(leftFrame);

        // 右侧下拉框：根据左侧选择动态变化
        QFrame* rightFrame = new QFrame;
        rightFrame->setFrameShape(QFrame::StyledPanel);
        rightFrame->setStyleSheet("background-color: #e0f0ff; border: 2px solid #87ceeb;");
        QVBoxLayout* rightLayout = new QVBoxLayout(rightFrame);
        rightLayout->setContentsMargins(5, 5, 5, 5);

        rightComboBox = new QComboBox;
        rightComboBox->setEditable(false); // 设置为不可编辑，确保显示选中的项
        rightComboBox->setStyleSheet(
            "QComboBox { background-color: white; border: 1px solid #ccc; padding: 8px; min-width: 120px; color: black; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: white; selection-background-color: #ff8c00; color: black; }"
            "QComboBox QAbstractItemView::item { padding: 8px; color: black; }"
            "QComboBox QAbstractItemView::item:selected { background-color: #ff8c00; color: white; }"
        );

        // 初始化右侧下拉框（默认显示"不分组"的选项）
        updateRightList(false);
        rightComboBox->update(); // 强制更新显示

        rightLayout->addWidget(rightComboBox);
        listLayout->addWidget(rightFrame);

        mainLayout->addLayout(listLayout);

        // 连接左侧下拉框选择变化事件
        connect(leftComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
            QString text = leftComboBox->itemText(index);
            bool isGroup = (text == "小组");

            // 更新右侧下拉框
            updateRightList(isGroup);
        });

        // 底部按钮
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(10);

        QString grayStyle = "background-color: #808080; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px;";
        QString greenStyle = "background-color: green; color: white; padding: 6px 12px; border-radius: 4px; font-size: 14px;";

        midtermComboBox = new QComboBox(this);
        midtermComboBox->setStyleSheet(
            "QComboBox { color: white; background-color: #808080; padding: 6px 12px; border-radius: 4px; font-size: 14px; }"
            "QComboBox QAbstractItemView { color: white; background-color: #5C5C5C; selection-background-color: #ff8c00; }"
        );
        subjectComboBox = new QComboBox(this);
        subjectComboBox->setStyleSheet(
            "QComboBox { color: white; background-color: #808080; padding: 6px 12px; border-radius: 4px; font-size: 14px; }"
            "QComboBox QAbstractItemView { color: white; background-color: #5C5C5C; selection-background-color: #ff8c00; }"
        );
        btnConfirm = new QPushButton("确定");

        btnConfirm->setStyleSheet(greenStyle);

        buttonLayout->addWidget(midtermComboBox);
        buttonLayout->addWidget(subjectComboBox);
        buttonLayout->addStretch();
        buttonLayout->addWidget(btnConfirm);

        mainLayout->addLayout(buttonLayout);

        // 连接确定按钮
        connect(btnConfirm, &QPushButton::clicked, this, [this]() {
            emit arrangeRequested(midtermComboBox->currentText(),
                                  subjectComboBox->currentText(),
                                  rightComboBox->currentText(),
                                  isGroupMode());
            accept();
        });

        // 下拉联动：选择文件后更新属性字段
        connect(midtermComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            updateAttributesFromSelectedFile();
        });
    }

    // 加载已下载的Excel文件并更新科目下拉框
    void loadExcelFiles(const QString& classId)
    {
        m_classId = classId;

        // 获取学校ID和班级ID
        UserInfo userInfo = CommonInfo::GetData();
        QString schoolId = userInfo.schoolId;

        if (schoolId.isEmpty() || classId.isEmpty()) {
            qDebug() << "学校ID或班级ID为空，无法加载Excel文件";
            return;
        }

        // 构建Excel文件目录路径
        QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
        QString schoolDir = baseDir + "/" + schoolId;
        QString classDir = schoolDir + "/" + classId;

        QDir dir(classDir);
        if (!dir.exists()) {
            qDebug() << "Excel文件目录不存在:" << classDir;
            return;
        }

        // 获取目录中的所有Excel文件
        QStringList filters;
        filters << "*.xlsx" << "*.xls" << "*.csv";
        QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

        qDebug() << "找到" << fileList.size() << "个Excel文件";

        // 收集所有Excel文件
        midtermComboBox->clear();
        subjectComboBox->clear();
        m_excelFileMap.clear();

        for (const QFileInfo& fileInfo : fileList) {
            QString filePath = fileInfo.absoluteFilePath();
            QString fileName = fileInfo.baseName(); // 去掉扩展名的文件名

            midtermComboBox->addItem(fileName);
            subjectComboBox->addItem(fileName);
            m_excelFileMap[fileName] = filePath;
        }

        // 如果有Excel文件，默认选中第一个
        if (midtermComboBox->count() > 0) {
            midtermComboBox->setCurrentIndex(0);
        }
        if (subjectComboBox->count() > 0) {
            subjectComboBox->setCurrentIndex(0);
        }

        // 加载首个文件的字段
        updateAttributesFromSelectedFile();
    }

    void setClassId(const QString& classId) { m_classId = classId; }

    // 获取选中的左侧选项（true=小组, false=不分组）
    bool isGroupMode() const
    {
        return leftComboBox->currentText() == "小组";
    }

    // 获取选中的右侧选项文本
    QString getSelectedMethod() const
    {
        return rightComboBox->currentText();
    }

protected:
    void showEvent(QShowEvent* event) override
    {
        QDialog::showEvent(event);
        if (!m_classId.isEmpty()) {
            loadExcelFiles(m_classId);
        }
    }

private:
    // 读取指定文件的表头并更新右侧下拉框
    void updateAttributesFromSelectedFile()
    {
        subjectComboBox->clear();
        QString fileName = midtermComboBox->currentText();
        if (fileName.isEmpty() || !m_excelFileMap.contains(fileName)) return;

        QString filePath = m_excelFileMap[fileName];
        QStringList headers;
        if (!readHeaders(filePath, headers)) return;

        for (const QString& h : headers) {
            QString trimmed = h.trimmed();
            if (trimmed.isEmpty()) continue;
            // 排除常规标识列
            if (trimmed == "学号" || trimmed == "姓名" || trimmed == "小组" || trimmed == "组号") continue;
            subjectComboBox->addItem(trimmed);
        }
        if (subjectComboBox->count() > 0) {
            subjectComboBox->setCurrentIndex(0);
        }
    }

    // 读取表头（首行），支持 xlsx/xls/csv
    bool readHeaders(const QString& filePath, QStringList& headers)
    {
        QFileInfo info(filePath);
        QString suffix = info.suffix().toLower();
        if (suffix == "xlsx" || suffix == "xls") {
            QXlsx::Document xlsx(filePath);
            int col = 1;
            while (true) {
                QString val = xlsx.read(1, col).toString();
                if (val.isEmpty()) break;
                headers << val;
                ++col;
            }
            return !headers.isEmpty();
        } else if (suffix == "csv") {
            QFile f(filePath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
            QTextStream ts(&f);
            ts.setCodec("UTF-8");
            if (!ts.atEnd()) {
                QString line = ts.readLine();
                headers = line.split(',', Qt::KeepEmptyParts);
            }
            f.close();
            return !headers.isEmpty();
        }
        return false;
    }

    void updateRightList(bool isGroup)
    {
        rightComboBox->clear();

        if (isGroup) {
            // 小组模式：2人组、4人组、6人组
            rightComboBox->addItem("2人组排座");
            rightComboBox->addItem("4人组排座");
            rightComboBox->addItem("6人组排座");
            // 默认选择第一个（2人组排座）
            rightComboBox->setCurrentIndex(0);
        } else {
            // 不分组模式：随机排座、倒序、正序
            rightComboBox->addItem("随机排座");
            rightComboBox->addItem("倒序");
            rightComboBox->addItem("正序");
            // 默认选择最后一个（正序）
            rightComboBox->setCurrentIndex(2);
        }
    }

private:
    QComboBox* leftComboBox;
    QComboBox* rightComboBox;
    QComboBox* midtermComboBox;
    QComboBox* subjectComboBox;
    QPushButton* btnConfirm;
    QString m_classId;
    QMap<QString, QString> m_excelFileMap;

signals:
    void arrangeRequested(const QString& fileName, const QString& fieldName, const QString& mode, bool isGroupMode);
};

