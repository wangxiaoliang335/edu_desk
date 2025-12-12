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
#include "GroupScoreDialog.h"
#include "xlsxdocument.h"
#include "CommonInfo.h"
#include <QRegularExpression>
#include <QMap>
#include <QSet>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QDate>
#include "ScoreHeaderIdStorage.h"

// 前向声明
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
        setWindowTitle("学生统计表导入");
        resize(300, 200);
        setStyleSheet("background-color: #808080;");
        
        // 确保窗口可以显示
        setAttribute(Qt::WA_DeleteOnClose, false);
        
        // 启用鼠标跟踪以检测鼠标进入/离开
        setMouseTracking(true);

        m_classid = classid;

        // 不再预先创建对话框，根据导入的Excel文件动态创建

        // 创建关闭按钮
        m_btnClose = new QPushButton("X", this);
        m_btnClose->setFixedSize(30, 30);
        m_btnClose->setStyleSheet(
            "QPushButton { background-color: #666666; color: white; font-weight: bold; font-size: 14px; border: none; border-radius: 4px; }"
            "QPushButton:hover { background-color: #777777; }"
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

        // 添加标题
        QLabel* lblTitle = new QLabel("学生统计表导入");
        lblTitle->setStyleSheet("background-color: #d3d3d3; color: black; font-size: 16px; font-weight: bold; padding: 8px; border-radius: 4px;");
        lblTitle->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(lblTitle);

        // 保存主布局的引用，用于动态添加按钮
        m_mainLayout = mainLayout;

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
        
        // 扫描并加载已下载的Excel文件（在初始化时调用）
        loadDownloadedExcelFiles();
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
        QString excelFileName = fileInfo.fileName(); // 提前获取文件名，避免重复定义
        
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
        bool isGroupScore = false; // 小组管理表
        
        // 检查第一列是否是"组号"或"小组"来判断是否为小组管理表
        bool hasGroupNumber = !headers.isEmpty() && (headers[0] == "组号" || headers[0] == "小组");
        
        // 先判断是否包含"小组"列来区分是带小组的表还是不带小组的表
        bool hasGroup = headers.contains("小组");
        
        // 只检查学号和姓名字段，其他字段都是属性字段，由服务器处理
        bool hasStudentId = headers.contains("学号");
        bool hasStudentName = headers.contains("姓名");
        
        // 如果第一列是"组号"或"小组"，且包含学号和姓名，则是小组管理表
        if (hasGroupNumber && hasStudentId && hasStudentName) {
            isGroupScore = true;
        }
        // 如果带"小组"列（但不是第一列），且包含学号和姓名，则是学生体质统计表（带小组的表）
        else if (hasGroup && hasStudentId && hasStudentName) {
            isStudentPhysique = true;
        }
        // 如果不带"小组"列，且包含学号和姓名，则是期中成绩单（不带小组的表）
        else if (!hasGroup && hasStudentId && hasStudentName) {
            isMidtermGrade = true;
        }

        // Excel文件名已在上面获取
        
        // 计算当前学期
        QDate currentDate = QDate::currentDate();
        int year = currentDate.year();
        int month = currentDate.month();
        QString term;
        if (month >= 9 || month <= 1) {
            if (month >= 9) {
                term = QString("%1-%2-1").arg(year).arg(year + 1);
            } else {
                term = QString("%1-%2-1").arg(year - 1).arg(year);
            }
        } else {
            term = QString("%1-%2-2").arg(year - 1).arg(year);
        }
        
        // 检查是否已存在相同文件名和学期的按钮
        QString existingTerm = m_fileNameToTermMap.value(excelFileName);
        if (!existingTerm.isEmpty() && existingTerm == term && m_dialogMap.contains(excelFileName)) {
            // 已存在相同文件名和学期的按钮，以导入的为准，覆盖已有对话框的数据
            QDialog* existingDialog = m_dialogMap[excelFileName];
            if (existingDialog) {
                bool updated = false;
                
                // 尝试更新已有对话框（如果类型匹配）
                if (isMidtermGrade) {
                    MidtermGradeDialog* midtermDlg = qobject_cast<MidtermGradeDialog*>(existingDialog);
                    if (midtermDlg) {
                        // 比对并更新姓名（以座位表为准）
                        updateNamesFromSeatInfo(headers, dataRows);
                        // 重新导入数据
                        midtermDlg->importData(headers, dataRows, fileName);
                        qDebug() << "已更新已有对话框的数据:" << excelFileName << "学期:" << term;
                        
                        QMessageBox::information(this, "更新成功", 
                            QString("已成功更新期中成绩单！\n共%1行数据。").arg(dataRows.size()));
                        midtermDlg->show();
                        midtermDlg->raise();
                        midtermDlg->activateWindow();
                        updated = true;
                    }
                } else if (isStudentPhysique) {
                    StudentPhysiqueDialog* physiqueDlg = qobject_cast<StudentPhysiqueDialog*>(existingDialog);
                    if (physiqueDlg) {
                        // 重新导入数据
                        physiqueDlg->importData(headers, dataRows, fileName);
                        qDebug() << "已更新已有对话框的数据:" << excelFileName << "学期:" << term;
                        
                        QMessageBox::information(this, "更新成功", 
                            QString("已成功更新学生体质统计表！\n共%1行数据。").arg(dataRows.size()));
                        physiqueDlg->show();
                        physiqueDlg->raise();
                        physiqueDlg->activateWindow();
                        updated = true;
                    }
                } else if (isGroupScore) {
                    GroupScoreDialog* groupScoreDlg = qobject_cast<GroupScoreDialog*>(existingDialog);
                    if (groupScoreDlg) {
                        // 重新导入数据
                        groupScoreDlg->importData(headers, dataRows, fileName);
                        qDebug() << "已更新已有对话框的数据:" << excelFileName << "学期:" << term;
                        
                        QMessageBox::information(this, "更新成功", 
                            QString("已成功更新小组管理表！\n共%1行数据。").arg(dataRows.size()));
                        groupScoreDlg->show();
                        groupScoreDlg->raise();
                        groupScoreDlg->activateWindow();
                        updated = true;
                    }
                }
                
                // 如果类型不匹配，删除旧的对话框和按钮，创建新的
                if (!updated) {
                    qDebug() << "对话框类型不匹配，删除旧的并创建新的:" << excelFileName;
                    // 找到对应的按钮
                    QPushButton* existingBtn = nullptr;
                    for (auto it = m_buttonToFileNameMap.begin(); it != m_buttonToFileNameMap.end(); ++it) {
                        if (it.value() == excelFileName) {
                            existingBtn = it.key();
                            break;
                        }
                    }
                    
                    // 删除旧的对话框
                    existingDialog->deleteLater();
                    m_dialogMap.remove(excelFileName);
                    m_fileNameToTermMap.remove(excelFileName);
                    
                    // 删除旧的按钮
                    if (existingBtn) {
                        m_buttonToFileNameMap.remove(existingBtn);
                        // 找到按钮所在的行布局并删除
                        QWidget* rowWidget = existingBtn->parentWidget();
                        if (rowWidget) {
                            QHBoxLayout* rowLayout = qobject_cast<QHBoxLayout*>(rowWidget->layout());
                            if (rowLayout) {
                                m_mainLayout->removeWidget(rowWidget);
                                rowWidget->deleteLater();
                            }
                        }
                    }
                    // 继续执行后面的创建新对话框逻辑
                } else {
                    // 更新成功，直接返回
                    return;
                }
            }
        }
        
        // 如果已经导入过这个文件（但学期不同），先删除旧的对话框和按钮，允许重新导入
        if (m_dialogMap.contains(excelFileName)) {
            QDialog* existingDlg = m_dialogMap[excelFileName];
            // 找到对应的按钮
            QPushButton* existingBtn = nullptr;
            for (auto it = m_buttonToFileNameMap.begin(); it != m_buttonToFileNameMap.end(); ++it) {
                if (it.value() == excelFileName) {
                    existingBtn = it.key();
                    break;
                }
            }
            
            // 删除旧的对话框
            if (existingDlg) {
                existingDlg->deleteLater();
            }
            m_dialogMap.remove(excelFileName);
            m_fileNameToTermMap.remove(excelFileName);
            
            // 删除旧的按钮
            if (existingBtn) {
                m_buttonToFileNameMap.remove(existingBtn);
                // 找到按钮所在的行布局并删除
                QWidget* rowWidget = existingBtn->parentWidget();
                if (rowWidget) {
                    QHBoxLayout* rowLayout = qobject_cast<QHBoxLayout*>(rowWidget->layout());
                    if (rowLayout) {
                        m_mainLayout->removeWidget(rowWidget);
                        rowWidget->deleteLater();
                    }
                }
            }
        }
        
        // 根据表格类型导入数据
        QDialog* dialog = nullptr;
        if (isMidtermGrade) {
            // 比对并更新姓名（以座位表为准）
            updateNamesFromSeatInfo(headers, dataRows);
            
            MidtermGradeDialog* midtermDlg = new MidtermGradeDialog(m_classid, this);
            // 传递Excel文件路径
            midtermDlg->importData(headers, dataRows, fileName);
            // 设置对话框标题为文件名（去掉扩展名）
            QFileInfo fileInfo(fileName);
            midtermDlg->setWindowTitle(fileInfo.baseName());
            
            // 从全局存储中获取 score_header_id 并设置
            int scoreHeaderId = ScoreHeaderIdStorage::getScoreHeaderId(m_classid, "期中考试", term);
            if (scoreHeaderId > 0) {
                midtermDlg->setScoreHeaderId(scoreHeaderId);
                qDebug() << "已为 MidtermGradeDialog 设置 score_header_id:" << scoreHeaderId;
            }
            dialog = midtermDlg;
            
            QMessageBox::information(this, "导入成功", 
                QString("已成功导入期中成绩单！\n共%1行数据。").arg(dataRows.size()));
            
            midtermDlg->show();
            
            // 延迟询问是否立即上传（等待对话框显示完成）
            QTimer::singleShot(300, this, [=]() {
                QMessageBox msgBox(midtermDlg);
                msgBox.setWindowTitle("上传确认");
                msgBox.setText("是否立即上传到服务器？");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                msgBox.button(QMessageBox::Yes)->setText("立即上传");
                msgBox.button(QMessageBox::No)->setText("稍后上传");
                
                int ret = msgBox.exec();
                if (ret == QMessageBox::Yes) {
                    // 直接调用上传方法
                    QMetaObject::invokeMethod(midtermDlg, "onUpload", Qt::QueuedConnection);
                }
            });
        } else if (isStudentPhysique) {
            StudentPhysiqueDialog* physiqueDlg = new StudentPhysiqueDialog(m_classid, this);
            // 传递Excel文件路径
            physiqueDlg->importData(headers, dataRows, fileName);
            // 设置对话框标题为文件名（去掉扩展名）
            QFileInfo fileInfo2(fileName);
            physiqueDlg->setWindowTitle(fileInfo2.baseName());
            
            // 从全局存储中获取 score_header_id 并设置
            int scoreHeaderId2 = ScoreHeaderIdStorage::getScoreHeaderId(m_classid, "期中考试", term);
            if (scoreHeaderId2 > 0) {
                physiqueDlg->setScoreHeaderId(scoreHeaderId2);
                qDebug() << "已为 StudentPhysiqueDialog 设置 score_header_id:" << scoreHeaderId2;
            }
            
            dialog = physiqueDlg;
            
            QMessageBox::information(this, "导入成功", 
                QString("已成功导入学生体质统计表！\n共%1行数据。").arg(dataRows.size()));
            
            physiqueDlg->show();
            
            // 延迟询问是否立即上传（等待对话框显示完成）
            QTimer::singleShot(300, this, [=]() {
                QMessageBox msgBox(physiqueDlg);
                msgBox.setWindowTitle("上传确认");
                msgBox.setText("是否立即上传到服务器？");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                msgBox.button(QMessageBox::Yes)->setText("立即上传");
                msgBox.button(QMessageBox::No)->setText("稍后上传");
                
                int ret = msgBox.exec();
                if (ret == QMessageBox::Yes) {
                    // 直接调用上传方法
                    QMetaObject::invokeMethod(physiqueDlg, "onUpload", Qt::QueuedConnection);
                }
            });
        } else if (isGroupScore) {
            GroupScoreDialog* groupScoreDlg = new GroupScoreDialog(m_classid, this);
            // 传递Excel文件路径
            groupScoreDlg->importData(headers, dataRows, fileName);
            // 设置对话框标题为文件名（去掉扩展名）
            QFileInfo fileInfo3(fileName);
            groupScoreDlg->setWindowTitle(fileInfo3.baseName());
            
            // 从全局存储中获取 score_header_id 并设置（小组管理表可能使用不同的 exam_name，这里先尝试期中考试）
            int scoreHeaderId3 = ScoreHeaderIdStorage::getScoreHeaderId(m_classid, "期中考试", term);
            if (scoreHeaderId3 > 0) {
                groupScoreDlg->setScoreHeaderId(scoreHeaderId3);
                qDebug() << "已为 GroupScoreDialog 设置 score_header_id:" << scoreHeaderId3;
            }
            
            dialog = groupScoreDlg;
            
            QMessageBox::information(this, "导入成功", 
                QString("已成功导入小组管理表！\n共%1行数据。").arg(dataRows.size()));
            
            groupScoreDlg->show();
            
            // 延迟询问是否立即上传（等待对话框显示完成）
            QTimer::singleShot(300, this, [=]() {
                QMessageBox msgBox(groupScoreDlg);
                msgBox.setWindowTitle("上传确认");
                msgBox.setText("是否立即上传到服务器？");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::Yes);
                msgBox.button(QMessageBox::Yes)->setText("立即上传");
                msgBox.button(QMessageBox::No)->setText("稍后上传");
                
                int ret = msgBox.exec();
                if (ret == QMessageBox::Yes) {
                    // 直接调用上传方法
                    QMetaObject::invokeMethod(groupScoreDlg, "onUpload", Qt::QueuedConnection);
                }
            });
        } else {
            QMessageBox::warning(this, "提示", 
                "无法识别表格类型！\n\n"
                "请确保表格包含'学号'和'姓名'列。");
            return;
        }
        
        // 如果成功创建对话框，则创建对应的按钮
        if (dialog) {
            // 保存对话框映射和学期信息
            m_dialogMap[excelFileName] = dialog;
            m_fileNameToTermMap[excelFileName] = term;
            
            // 创建按钮行（在"+"按钮之前插入）
            // 找到"+"按钮的位置
            int insertIndex = m_mainLayout->count() - 1; // "+"按钮是最后一个
            if (insertIndex < 0) insertIndex = 0;
            
            // 创建按钮行布局
            QHBoxLayout *rowLayout = new QHBoxLayout;
            rowLayout->setSpacing(0);

            QPushButton *btnTitle = new QPushButton(excelFileName);
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
            
            // 将按钮行插入到"+"按钮之前
            m_mainLayout->insertLayout(insertIndex, rowLayout);
            
            // 保存按钮映射
            m_buttonToFileNameMap[btnTitle] = excelFileName;
            
            // 连接按钮点击事件
            connect(btnTitle, &QPushButton::clicked, this, [=]() {
                if (dialog && dialog->isHidden()) {
                    dialog->show();
                    dialog->raise();
                    dialog->activateWindow();
                } else if (dialog && !dialog->isHidden()) {
                    dialog->hide();
                }
            });
            
            // 连接关闭按钮
            connect(btnClose, &QPushButton::clicked, this, [=]() {
                // 删除按钮和对话框
                m_dialogMap.remove(excelFileName);
                m_buttonToFileNameMap.remove(btnTitle);
                m_fileNameToTermMap.remove(excelFileName);
                
                // 从布局中移除按钮行
                m_mainLayout->removeItem(rowLayout);
                
                // 删除按钮和布局
                btnTitle->deleteLater();
                btnClose->deleteLater();
                rowLayout->deleteLater();
                
                // 删除对话框
                if (dialog) {
                    dialog->deleteLater();
                }
            });
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
    
    // 扫描并加载已下载的Excel文件
    void loadDownloadedExcelFiles();
    
    // 加载单个Excel文件并创建按钮
    void loadExcelFileAndCreateButton(const QString& filePath);

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
    // 保存主布局的引用，用于动态添加按钮
    QVBoxLayout* m_mainLayout = nullptr;
    
    // 使用QMap保存按钮和对话框的映射关系（Excel文件名 -> 对话框指针）
    QMap<QString, QDialog*> m_dialogMap; // Excel文件名 -> 对话框
    QMap<QPushButton*, QString> m_buttonToFileNameMap; // 按钮 -> Excel文件名
    // 存储按钮对应的学期信息（键：文件名，值：学期）
    QMap<QString, QString> m_fileNameToTermMap; // Excel文件名 -> 学期
    
    QString m_classid;
    QPushButton* m_btnClose = nullptr; // 关闭按钮
    QPoint m_dragPosition; // 用于窗口拖动
    QSet<QString> m_loadedFiles; // 已加载的文件路径，避免重复加载
};

// 扫描并加载已下载的Excel文件
// 现在支持扫描 group/ 和 student/ 两个子目录
inline void CustomListDialog::loadDownloadedExcelFiles()
{
    // 获取学校ID和班级ID
    UserInfo userInfo = CommonInfo::GetData();
    QString schoolId = userInfo.schoolId;
    QString classId = m_classid;
    
    if (schoolId.isEmpty() || classId.isEmpty()) {
        qDebug() << "学校ID或班级ID为空，无法加载已下载的Excel文件";
        return;
    }
    
    // 构建Excel文件目录路径
    QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
    QString schoolDir = baseDir + "/" + schoolId;
    QString classDir = schoolDir + "/" + classId;
    
    QStringList filters;
    filters << "*.xlsx" << "*.xls" << "*.csv";
    QFileInfoList fileList;
    
    // 扫描主目录（向后兼容）
    QDir dir(classDir);
    if (dir.exists()) {
        QFileInfoList mainFiles = dir.entryInfoList(filters, QDir::Files);
        fileList.append(mainFiles);
    }
    
    // 扫描 group/ 子目录
    QDir groupDir(classDir + "/group");
    if (groupDir.exists()) {
        QFileInfoList groupFiles = groupDir.entryInfoList(filters, QDir::Files);
        fileList.append(groupFiles);
    }
    
    // 扫描 student/ 子目录
    QDir studentDir(classDir + "/student");
    if (studentDir.exists()) {
        QFileInfoList studentFiles = studentDir.entryInfoList(filters, QDir::Files);
        fileList.append(studentFiles);
    }
    
    if (fileList.isEmpty()) {
        qDebug() << "Excel文件目录不存在或没有文件:" << classDir;
        return;
    }
    
    qDebug() << "找到" << fileList.size() << "个Excel文件";
    
    // 为每个文件创建按钮（如果还没有加载过）
    for (const QFileInfo& fileInfo : fileList) {
        QString filePath = fileInfo.absoluteFilePath();
        QString fileName = fileInfo.fileName();
        
        // 检查是否已经加载过
        if (m_loadedFiles.contains(filePath) || m_dialogMap.contains(fileName)) {
            continue;
        }
        
        // 加载文件并创建按钮
        loadExcelFileAndCreateButton(filePath);
    }
}

// 加载单个Excel文件并创建按钮
inline void CustomListDialog::loadExcelFileAndCreateButton(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "文件不存在:" << filePath;
        return;
    }
    
    QString fileName = fileInfo.fileName();
    QString suffix = fileInfo.suffix().toLower();
    
    // 读取Excel文件
    QStringList headers;
    QList<QStringList> dataRows;
    
    bool readSuccess = false;
    if (suffix == "xlsx" || suffix == "xls") {
        readSuccess = readExcelFile(filePath, headers, dataRows);
    } else if (suffix == "csv") {
        readSuccess = readCSVFile(filePath, headers, dataRows);
    }
    
    if (!readSuccess || headers.isEmpty()) {
        qWarning() << "无法读取Excel文件:" << filePath;
        return;
    }
    
    // 判断表格类型
    bool isMidtermGrade = false;
    bool isStudentPhysique = false;
    bool isGroupScore = false;
    
    // 检查第一列是否是"组号"或"小组"来判断是否为小组管理表
    bool hasGroupNumber = !headers.isEmpty() && (headers[0] == "组号" || headers[0] == "小组");
    bool hasGroup = headers.contains("小组");
    bool hasStudentId = headers.contains("学号");
    bool hasStudentName = headers.contains("姓名");
    
    // 如果第一列是"组号"或"小组"，且包含学号和姓名，则是小组管理表
    if (hasGroupNumber && hasStudentId && hasStudentName) {
        isGroupScore = true;
    }
    // 如果带"小组"列（但不是第一列），且包含学号和姓名，则是学生体质统计表
    else if (hasGroup && hasStudentId && hasStudentName) {
        isStudentPhysique = true;
    }
    // 如果不带"小组"列，且包含学号和姓名，则是期中成绩单
    else if (!hasGroup && hasStudentId && hasStudentName) {
        isMidtermGrade = true;
    } else {
        qWarning() << "无法识别表格类型:" << fileName;
        return;
    }
    
    // 计算当前学期
    QDate currentDate = QDate::currentDate();
    int year = currentDate.year();
    int month = currentDate.month();
    QString term;
    if (month >= 9 || month <= 1) {
        if (month >= 9) {
            term = QString("%1-%2-1").arg(year).arg(year + 1);
        } else {
            term = QString("%1-%2-1").arg(year - 1).arg(year);
        }
    } else {
        term = QString("%1-%2-2").arg(year - 1).arg(year);
    }
    
    // 检查是否已存在相同文件名和学期的按钮
    QString existingTerm = m_fileNameToTermMap.value(fileName);
    if (!existingTerm.isEmpty() && existingTerm == term && m_dialogMap.contains(fileName)) {
        // 已存在相同文件名和学期的按钮，更新已有对话框的数据
        QDialog* existingDialog = m_dialogMap[fileName];
        if (existingDialog) {
            if (isMidtermGrade) {
                MidtermGradeDialog* midtermDlg = qobject_cast<MidtermGradeDialog*>(existingDialog);
                if (midtermDlg) {
                    // 比对并更新姓名（以座位表为准）
                    updateNamesFromSeatInfo(headers, dataRows);
                    // 重新导入数据
                    midtermDlg->importData(headers, dataRows, filePath);
                    qDebug() << "已更新已有对话框的数据:" << fileName << "学期:" << term;
                    return;
                }
            } else if (isStudentPhysique) {
                StudentPhysiqueDialog* physiqueDlg = qobject_cast<StudentPhysiqueDialog*>(existingDialog);
                if (physiqueDlg) {
                    // 重新导入数据
                    physiqueDlg->importData(headers, dataRows, filePath);
                    qDebug() << "已更新已有对话框的数据:" << fileName << "学期:" << term;
                    return;
                }
            } else if (isGroupScore) {
                GroupScoreDialog* groupScoreDlg = qobject_cast<GroupScoreDialog*>(existingDialog);
                if (groupScoreDlg) {
                    // 重新导入数据
                    groupScoreDlg->importData(headers, dataRows, filePath);
                    qDebug() << "已更新已有对话框的数据:" << fileName << "学期:" << term;
                    return;
                }
            }
        }
    }
    
    // 根据表格类型导入数据
    QDialog* dialog = nullptr;
    if (isMidtermGrade) {
        // 比对并更新姓名（以座位表为准）
        updateNamesFromSeatInfo(headers, dataRows);
        
        MidtermGradeDialog* midtermDlg = new MidtermGradeDialog(m_classid, this);
        midtermDlg->importData(headers, dataRows, filePath);
        // 设置对话框标题为文件名（去掉扩展名）
        midtermDlg->setWindowTitle(fileInfo.baseName());
        
        // 从全局存储中获取 score_header_id 并设置
        int scoreHeaderId = ScoreHeaderIdStorage::getScoreHeaderId(m_classid, "期中考试", term);
        if (scoreHeaderId > 0) {
            midtermDlg->setScoreHeaderId(scoreHeaderId);
            qDebug() << "已为 MidtermGradeDialog 设置 score_header_id:" << scoreHeaderId;
        }
        
        dialog = midtermDlg;
    } else if (isStudentPhysique) {
        StudentPhysiqueDialog* physiqueDlg = new StudentPhysiqueDialog(m_classid, this);
        physiqueDlg->importData(headers, dataRows, filePath);
        // 设置对话框标题为文件名（去掉扩展名）
        physiqueDlg->setWindowTitle(fileInfo.baseName());
        
        // 从全局存储中获取 score_header_id 并设置（学生体质表可能使用不同的 exam_name，这里先尝试期中考试）
        int scoreHeaderId = ScoreHeaderIdStorage::getScoreHeaderId(m_classid, "期中考试", term);
        if (scoreHeaderId > 0) {
            physiqueDlg->setScoreHeaderId(scoreHeaderId);
            qDebug() << "已为 StudentPhysiqueDialog 设置 score_header_id:" << scoreHeaderId;
        }
        
        dialog = physiqueDlg;
    } else if (isGroupScore) {
        GroupScoreDialog* groupScoreDlg = new GroupScoreDialog(m_classid, this);
        groupScoreDlg->importData(headers, dataRows, filePath);
        // 设置对话框标题为文件名（去掉扩展名）
        groupScoreDlg->setWindowTitle(fileInfo.baseName());
        
        // 从全局存储中获取 score_header_id 并设置（小组管理表可能使用不同的 exam_name，这里先尝试期中考试）
        int scoreHeaderId = ScoreHeaderIdStorage::getScoreHeaderId(m_classid, "期中考试", term);
        if (scoreHeaderId > 0) {
            groupScoreDlg->setScoreHeaderId(scoreHeaderId);
            qDebug() << "已为 GroupScoreDialog 设置 score_header_id:" << scoreHeaderId;
        }
        
        dialog = groupScoreDlg;
    }
    
    if (!dialog) {
        return;
    }
    
    // 标记为已加载
    m_loadedFiles.insert(filePath);
    
    // 保存对话框映射和学期信息
    m_dialogMap[fileName] = dialog;
    m_fileNameToTermMap[fileName] = term;
    
    // 创建按钮行（在"+"按钮之前插入）
    int insertIndex = m_mainLayout->count() - 1; // "+"按钮是最后一个
    if (insertIndex < 0) insertIndex = 0;
    
    // 创建按钮行布局
    QHBoxLayout *rowLayout = new QHBoxLayout;
    rowLayout->setSpacing(0);
    
    QPushButton *btnTitle = new QPushButton(fileName);
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
    
    // 将按钮行插入到"+"按钮之前
    m_mainLayout->insertLayout(insertIndex, rowLayout);
    
    // 保存按钮映射
    m_buttonToFileNameMap[btnTitle] = fileName;
    
    // 连接按钮点击事件
    connect(btnTitle, &QPushButton::clicked, this, [=]() {
        if (dialog && dialog->isHidden()) {
            dialog->show();
            dialog->raise();
            dialog->activateWindow();
        } else if (dialog && !dialog->isHidden()) {
            dialog->hide();
        }
    });
    
    // 连接关闭按钮
    connect(btnClose, &QPushButton::clicked, this, [=]() {
        // 删除按钮和对话框
        m_dialogMap.remove(fileName);
        m_buttonToFileNameMap.remove(btnTitle);
        m_fileNameToTermMap.remove(fileName);
        m_loadedFiles.remove(filePath);
        
        // 从布局中移除按钮行
        m_mainLayout->removeItem(rowLayout);
        
        // 删除按钮和布局
        btnTitle->deleteLater();
        btnClose->deleteLater();
        rowLayout->deleteLater();
        
        // 删除对话框
        if (dialog) {
            dialog->deleteLater();
        }
    });
    
    qDebug() << "已加载Excel文件并创建按钮:" << fileName;
}
