#include "ScheduleDialog.h"
#include "RandomCallDialog.h"
#include <QMessageBox>

// 实现随机点名按钮的连接逻辑
void ScheduleDialog::connectRandomCallButton(QPushButton* btnRandom, QTableWidget* seatTable)
{
    if (!btnRandom) return;
    
    connect(btnRandom, &QPushButton::clicked, this, [this, seatTable]() {
        // 检查是否有学生数据
        if (m_students.isEmpty()) {
            QMessageBox::information(this, "提示", "请先上传期中成绩表并排好座位！");
            return;
        }
        
        if (!randomCallDlg) {
            randomCallDlg = new RandomCallDialog(this);
            randomCallDlg->setStudentData(m_students);
            randomCallDlg->setSeatTable(seatTable);
            // 连接学生成绩更新信号
            connect(randomCallDlg, &RandomCallDialog::studentScoreUpdated, this, [this](const QString& studentId, double newScore) {
                // 更新本地学生数据
                for (auto& student : m_students) {
                    if (student.id == studentId) {
                        student.score = newScore;
                        break;
                    }
                }
                // 如果热力图已设置，需要更新座位颜色
                if (m_heatmapType == 1 || m_heatmapType == 2) {
                    updateSeatColors();
                }
            });
        } else {
            randomCallDlg->setStudentData(m_students);
            randomCallDlg->setSeatTable(seatTable);
        }
        randomCallDlg->show();
        randomCallDlg->raise();
        randomCallDlg->activateWindow();
    });
}

