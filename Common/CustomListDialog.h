#pragma once
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QIODevice>
#include <QFileInfo>
#include <QTimer>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPoint>
#include <QCursor>
#include <QRect>
#include <QDesktopWidget>
#include "MidtermGradeDialog.h"
#include "StudentPhysiqueDialog.h"
#include "xlsxdocument.h"
#include <QRegularExpression>
#include <QMap>
#include <QDebug>

// 前向声明
class ScheduleDialog;
struct SeatInfo;

class CustomListDialog : public QDialog
{
    Q_OBJECT
public:
    CustomListDialog(QString classid, QWidget *parent = nullptr) : QDialog(parent)
    {
        // 去掉标题栏，但保持窗口属性
        // 使用 Dialog 标志确保窗口可以正常显示
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
        setWindowTitle("列表管理");
        resize(300, 200);
        setStyleSheet("background-color: #808080;");
        
        // 确保窗口可以显示
        setAttribute(Qt::WA_DeleteOnClose, false);
        
        // 启用鼠标跟踪以检测鼠标进入/离开
        setMouseTracking(true);

        m_classid = classid;

        m_midtermGradeDlg = new MidtermGradeDialog(classid, this);
        m_studentPhysiqueDlg = new StudentPhysiqueDialog(this);

        // 创建关闭按钮
        m_btnClose = new QPushButton("X", this);
        m_btnClose->setFixedSize(30, 30);
        m_btnClose->setStyleSheet(
            "QPushButton { background-color: orange; color: white; font-weight:bold; font-size: 14px; border: 1px solid #555; border-radius: 4px; }"
            "QPushButton:hover { background-color: #cc6600; }"
        );
        m_btnClose->hide(); // 初始隐藏
        connect(m_btnClose, &QPushButton::clicked, this, &QDialog::close);
        
        // 为关闭按钮安装事件过滤器，确保鼠标在按钮上时不会隐藏
        m_btnClose->installEventFilter(this);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);
        mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        // 增加顶部边距，为关闭按钮留出空间（关闭按钮高度30，位置y=5，所以顶部至少需要40）
        mainLayout->setContentsMargins(10, 40, 10, 10);

        // 添加两行列表项
        QPushButton* btnMidterm = addRow(mainLayout, "期中成绩单");
        QPushButton* btnPhysique = addRow(mainLayout, "学生体质统计表");

        // 连接期中成绩单按钮点击事件
        connect(btnMidterm, &QPushButton::clicked, this, [=]() {
            if (m_midtermGradeDlg && m_midtermGradeDlg->isHidden()) {
                m_midtermGradeDlg->show();
            } else if (m_midtermGradeDlg && !m_midtermGradeDlg->isHidden()) {
                m_midtermGradeDlg->hide();
            } else {
                m_midtermGradeDlg = new MidtermGradeDialog(classid, this);
                m_midtermGradeDlg->show();
            }
        });

        // 连接学生体质统计表按钮点击事件
        connect(btnPhysique, &QPushButton::clicked, this, [=]() {
            if (m_studentPhysiqueDlg && m_studentPhysiqueDlg->isHidden()) {
                m_studentPhysiqueDlg->show();
            } else if (m_studentPhysiqueDlg && !m_studentPhysiqueDlg->isHidden()) {
                m_studentPhysiqueDlg->hide();
            } else {
                m_studentPhysiqueDlg = new StudentPhysiqueDialog(this);
                m_studentPhysiqueDlg->show();
            }
        });

        // 底部 "+"" 按钮
        QPushButton *btnAdd = new QPushButton("+");
        btnAdd->setFixedSize(40,40);
        btnAdd->setStyleSheet(
            "QPushButton { background-color: orange; color:white; font-weight:bold; font-size: 18px; border: 1px solid #555; }"
            "QPushButton:hover { background-color: #cc6600; }"
        );
        mainLayout->addSpacing(20);
        mainLayout->addWidget(btnAdd, 0, Qt::AlignCenter);

        // 连接"+"按钮点击事件，导入Excel表格
        connect(btnAdd, &QPushButton::clicked, this, &CustomListDialog::onImportExcel);
    }
    
protected:
    // 重写鼠标事件以实现窗口拖动
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton && !m_dragPosition.isNull()) {
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    }
    
    // 鼠标进入窗口时显示关闭按钮
    void enterEvent(QEvent *event) override
    {
        if (m_btnClose) {
            m_btnClose->show();
        }
        QDialog::enterEvent(event);
    }
    
    // 鼠标离开窗口时隐藏关闭按钮
    void leaveEvent(QEvent *event) override
    {
        // 检查鼠标是否真的离开了窗口（包括关闭按钮）
        QPoint globalPos = QCursor::pos();
        QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
        if (!widgetRect.contains(globalPos) && m_btnClose) {
            // 如果鼠标不在窗口内，检查是否在关闭按钮上
            QRect btnRect = QRect(m_btnClose->mapToGlobal(QPoint(0, 0)), m_btnClose->size());
            if (!btnRect.contains(globalPos)) {
                m_btnClose->hide();
            }
        }
        QDialog::leaveEvent(event);
    }
    
    // 事件过滤器，处理关闭按钮的鼠标事件
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == m_btnClose) {
            if (event->type() == QEvent::Enter) {
                // 鼠标进入关闭按钮时确保显示
                m_btnClose->show();
            } else if (event->type() == QEvent::Leave) {
                // 鼠标离开关闭按钮时，检查是否还在窗口内
                QPoint globalPos = QCursor::pos();
                QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
                if (!widgetRect.contains(globalPos)) {
                    m_btnClose->hide();
                }
            }
        }
        return QDialog::eventFilter(obj, event);
    }
    
    // 窗口大小改变时更新关闭按钮位置
    void resizeEvent(QResizeEvent *event) override
    {
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
        }
        QDialog::resizeEvent(event);
    }
    
    // 窗口显示时更新关闭按钮位置
    void showEvent(QShowEvent *event) override
    {
        if (m_btnClose) {
            m_btnClose->move(width() - 35, 5);
        }
        
        // 确保窗口位置在屏幕可见区域内
        QRect screenGeometry = QApplication::desktop()->availableGeometry();
        QRect windowGeometry = geometry();
        
        // 如果窗口完全在屏幕外，移动到屏幕中央
        if (!screenGeometry.intersects(windowGeometry)) {
            move(screenGeometry.center() - QPoint(windowGeometry.width() / 2, windowGeometry.height() / 2));
        }
        
        // 确保窗口显示在最前面
        raise();
        activateWindow();
        QDialog::showEvent(event);
    }

private slots:
    void onImportExcel()
    {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "导入表格文件",
            "",
            "Excel文件 (*.xlsx *.xls);;CSV文件 (*.csv);;所有文件 (*.*)"
        );

        if (fileName.isEmpty()) {
            return;
        }

        // 检查文件扩展名
        QFileInfo fileInfo(fileName);
        QString suffix = fileInfo.suffix().toLower();
        
        QStringList headers;
        QList<QStringList> dataRows;
        
        // 根据文件类型读取数据
        if (suffix == "xlsx" || suffix == "xls") {
            // 读取 Excel 文件
            if (!readExcelFile(fileName, headers, dataRows)) {
                QMessageBox::critical(this, "错误", "无法读取 Excel 文件！");
                return;
            }
        } else if (suffix == "csv") {
            // 读取 CSV 文件
            if (!readCSVFile(fileName, headers, dataRows)) {
                QMessageBox::critical(this, "错误", "无法读取 CSV 文件！");
                return;
            }
        } else {
            QMessageBox::warning(this, "提示", "不支持的文件格式！\n请选择 Excel (*.xlsx, *.xls) 或 CSV (*.csv) 文件。");
            return;
        }
        
        if (headers.isEmpty()) {
            QMessageBox::warning(this, "提示", "文件为空或格式不正确！");
            return;
        }
        
        // 判断表格类型
        bool isMidtermGrade = false; // 期中成绩单
        bool isStudentPhysique = false; // 学生体质统计表
        
        // 检查是否是期中成绩单（包含：学号、姓名、语文、数学、英语、总分）
        if (headers.contains("学号") && headers.contains("姓名") && 
            headers.contains("语文") && headers.contains("数学") && 
            headers.contains("英语") && headers.contains("总分") &&
            !headers.contains("小组")) {
            isMidtermGrade = true;
        }
        // 检查是否是学生体质统计表（包含：小组、学号、姓名等）
        else if (headers.contains("小组") && headers.contains("学号") && 
                 headers.contains("姓名") && headers.contains("小组总分")) {
            isStudentPhysique = true;
        }

        // 根据表格类型导入数据
        if (isMidtermGrade) {
            // 比对并更新姓名（以座位表为准）
            updateNamesFromSeatInfo(headers, dataRows);
            
            if (!m_midtermGradeDlg) {
                m_midtermGradeDlg = new MidtermGradeDialog(m_classid, this);
            }
            m_midtermGradeDlg->importData(headers, dataRows);
            
            QMessageBox::information(this, "导入成功", 
                QString("已成功导入期中成绩单！\n共%1行数据。").arg(dataRows.size()));
            
            m_midtermGradeDlg->show();
            
            // 延迟询问是否立即上传（等待对话框显示完成）
            QTimer::singleShot(300, this, [=]() {
                QMessageBox msgBox(m_midtermGradeDlg);
                msgBox.setWindowTitle("上传确认");
                msgBox.setText("是否立即上传到服务器？");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                msgBox.button(QMessageBox::Yes)->setText("立即上传");
                msgBox.button(QMessageBox::No)->setText("稍后上传");
                
                int ret = msgBox.exec();
                if (ret == QMessageBox::Yes) {
                    // 直接调用上传方法
                    QMetaObject::invokeMethod(m_midtermGradeDlg, "onUpload", Qt::QueuedConnection);
                }
            });
        } else if (isStudentPhysique) {
            if (!m_studentPhysiqueDlg) {
                m_studentPhysiqueDlg = new StudentPhysiqueDialog(this);
            }
            m_studentPhysiqueDlg->importData(headers, dataRows);
            
            QMessageBox::information(this, "导入成功", 
                QString("已成功导入学生体质统计表！\n共%1行数据。").arg(dataRows.size()));
            
            m_studentPhysiqueDlg->show();
            
            // 延迟询问是否立即上传（等待对话框显示完成）
            QTimer::singleShot(300, this, [=]() {
                QMessageBox msgBox(m_studentPhysiqueDlg);
                msgBox.setWindowTitle("上传确认");
                msgBox.setText("是否立即上传到服务器？");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                msgBox.button(QMessageBox::Yes)->setText("立即上传");
                msgBox.button(QMessageBox::No)->setText("稍后上传");
                
                int ret = msgBox.exec();
                if (ret == QMessageBox::Yes) {
                    // 直接调用上传方法
                    QMetaObject::invokeMethod(m_studentPhysiqueDlg, "onUpload", Qt::QueuedConnection);
                }
            });
        } else {
            QMessageBox::warning(this, "提示", 
                "无法识别表格类型！\n\n"
                "期中成绩单应包含：学号、姓名、语文、数学、英语、总分\n"
                "学生体质统计表应包含：小组、学号、姓名、小组总分\n\n"
                "请确保文件格式正确，列名匹配上述要求。");
        }
    }

private:
    // 读取 Excel 文件
    bool readExcelFile(const QString& fileName, QStringList& headers, QList<QStringList>& dataRows)
    {
        using namespace QXlsx;
        
        // 检查文件是否存在
        if (!QFile::exists(fileName)) {
            return false;
        }
        
        Document xlsx(fileName);
        
        // 读取第一行作为表头
        int col = 1;
        headers.clear();
        while (true) {
            QVariant cellValue = xlsx.read(1, col);
            if (cellValue.isNull()) {
                // 如果第一列就是空的，可能是文件格式问题
                if (col == 1) {
                    return false;
                }
                break;
            }
            QString cellText = cellValue.toString().trimmed();
            if (cellText.isEmpty() && col > 1) {
                // 遇到空列，停止读取表头
                break;
            }
            headers.append(cellText);
            ++col;
            // 限制最大列数，防止无限循环
            if (col > 1000) {
                break;
            }
        }
        
        if (headers.isEmpty()) {
            return false;
        }
        
        // 读取数据行（从第2行开始）
        dataRows.clear();
        int row = 2;
        int maxRows = 10000; // 限制最大行数，防止无限循环
        
        while (row <= maxRows) {
            QStringList rowData;
            bool hasData = false;
            
            for (int c = 1; c <= headers.size(); ++c) {
                QVariant cellValue = xlsx.read(row, c);
                QString cellText = cellValue.isNull() ? "" : cellValue.toString().trimmed();
                rowData.append(cellText);
                if (!cellText.isEmpty()) {
                    hasData = true;
                }
            }
            
            if (!hasData) {
                // 连续遇到空行，停止读取（最多检查3行）
                bool allEmpty = true;
                for (int checkRow = row; checkRow < row + 3 && checkRow <= maxRows; ++checkRow) {
                    for (int c = 1; c <= headers.size(); ++c) {
                        QVariant cellValue = xlsx.read(checkRow, c);
                        if (!cellValue.isNull() && !cellValue.toString().trimmed().isEmpty()) {
                            allEmpty = false;
                            break;
                        }
                    }
                    if (!allEmpty) break;
                }
                if (allEmpty) {
                    break; // 确实都是空行，停止读取
                }
            }
            
            dataRows.append(rowData);
            ++row;
        }
        
        return true;
    }
    
    // 读取 CSV 文件
    bool readCSVFile(const QString& fileName, QStringList& headers, QList<QStringList>& dataRows)
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");
        
        QString content = in.readAll();
        if (content.startsWith("\xEF\xBB\xBF")) {
            content = content.mid(3);
        }
        
        // 解析 CSV 内容（处理引号和逗号）
        QStringList lines;
        QString currentLine;
        bool inQuotes = false;
        
        for (int i = 0; i < content.length(); ++i) {
            QChar c = content[i];
            if (c == '"') {
                inQuotes = !inQuotes;
                currentLine += c;
            } else if (c == '\n' && !inQuotes) {
                if (!currentLine.isEmpty()) {
                    lines.append(currentLine);
                }
                currentLine.clear();
            } else {
                currentLine += c;
            }
        }
        if (!currentLine.isEmpty()) {
            lines.append(currentLine);
        }
        
        if (lines.isEmpty()) {
            file.close();
            return false;
        }

        // 解析 CSV 行的辅助函数
        auto parseCSVLine = [](const QString& line) -> QStringList {
            QStringList fields;
            QString currentField;
            bool inQuotes = false;
            
            for (int i = 0; i < line.length(); ++i) {
                QChar c = line[i];
                if (c == '"') {
                    if (i + 1 < line.length() && line[i + 1] == '"') {
                        // 双引号转义
                        currentField += '"';
                        ++i;
                    } else {
                        inQuotes = !inQuotes;
                    }
                } else if (c == ',' && !inQuotes) {
                    fields.append(currentField.trimmed());
                    currentField.clear();
                } else {
                    currentField += c;
                }
            }
            fields.append(currentField.trimmed()); // 添加最后一个字段
            return fields;
        };
        
        // 读取表头
        headers = parseCSVLine(lines[0]);
        
        // 读取数据行
        dataRows.clear();
        for (int i = 1; i < lines.size(); ++i) {
            QStringList fields = parseCSVLine(lines[i]);
            if (!fields.isEmpty() && !fields[0].trimmed().isEmpty()) { // 跳过空行
                dataRows.append(fields);
            }
        }
        
        file.close();
        return true;
    }
    
    // 比对并更新姓名（以座位表为准）
    void updateNamesFromSeatInfo(const QStringList& headers, QList<QStringList>& dataRows);
    
    // 从字符串中提取纯姓名（去掉数字部分）
    // 例如："姜凯文13-5/14" -> "姜凯文"
    QString extractPureName(const QString& nameStr);

private:
    QPushButton* addRow(QVBoxLayout *parentLayout, const QString &text)
    {
        QHBoxLayout *rowLayout = new QHBoxLayout;
        rowLayout->setSpacing(0);

        QPushButton *btnTitle = new QPushButton(text);
        btnTitle->setStyleSheet(
            "QPushButton { background-color: green; color: white; font-size: 14px; padding: 4px; border: 1px solid #555; }"
            "QPushButton:hover { background-color: darkgreen; }"
        );
        btnTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QPushButton *btnClose = new QPushButton("X");
        btnClose->setFixedWidth(30);
        btnClose->setStyleSheet(
            "QPushButton { background-color: orange; color: white; font-weight:bold; border: 1px solid #555; }"
            "QPushButton:hover { background-color: #cc6600; }"
        );

        rowLayout->addWidget(btnTitle);
        rowLayout->addWidget(btnClose);
        parentLayout->addLayout(rowLayout);
        
        return btnTitle;
    }

private:
    MidtermGradeDialog* m_midtermGradeDlg = nullptr;
    StudentPhysiqueDialog* m_studentPhysiqueDlg = nullptr;
    QString m_classid;
    QPushButton* m_btnClose = nullptr; // 关闭按钮
    QPoint m_dragPosition; // 用于窗口拖动
};
