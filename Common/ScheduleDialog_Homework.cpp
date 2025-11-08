#include "ScheduleDialog.h"
#include "HomeworkEditDialog.h"
#include "HomeworkViewDialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QDate>

// 实现作业按钮的连接逻辑
void ScheduleDialog::connectHomeworkButton(QPushButton* btnTask)
{
    if (!btnTask) return;
    
    connect(btnTask, &QPushButton::clicked, this, [this]() {
        // 教师端：打开编辑作业窗口
        if (!homeworkEditDlg) {
            homeworkEditDlg = new HomeworkEditDialog(this);
            homeworkEditDlg->setDate(QDate::currentDate());
            
            // 连接发布信号
            connect(homeworkEditDlg, &HomeworkEditDialog::homeworkPublished, this, 
                [this](const QDate& date, const QMap<QString, QString>& content) {
                    qDebug() << "作业已发布，日期:" << date.toString("yyyy-MM-dd");
                    for (auto it = content.begin(); it != content.end(); ++it) {
                        qDebug() << "科目:" << it.key() << "，内容:" << it.value();
                    }
                    // 这里可以添加将作业保存到服务器或本地存储的逻辑
                    // TODO: 保存作业到服务器
                });
        }
        homeworkEditDlg->setDate(QDate::currentDate());
        homeworkEditDlg->show();
        homeworkEditDlg->raise();
        homeworkEditDlg->activateWindow();
    });
}

// 显示作业展示窗口（班级端）
void ScheduleDialog::showHomeworkViewDialog()
{
    if (!homeworkViewDlg) {
        homeworkViewDlg = new HomeworkViewDialog(this);
        
        // 这里可以从服务器或本地存储加载作业内容
        // TODO: 从服务器加载作业
        QMap<QString, QString> homeworkContent;
        // homeworkContent["语文"] = "背诵《荷塘月色》第二段";
        // homeworkContent["数学"] = "做同步练习册第二节选择题";
        
        homeworkViewDlg->setDate(QDate::currentDate());
        homeworkViewDlg->setHomeworkContent(homeworkContent);
    }
    homeworkViewDlg->setDate(QDate::currentDate());
    homeworkViewDlg->show();
    homeworkViewDlg->raise();
    homeworkViewDlg->activateWindow();
}

