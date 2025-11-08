#include "ScheduleDialog.h"
#include "HeatmapSegmentDialog.h"
#include "HeatmapViewDialog.h"
#include "HeatmapTypes.h"
#include <QMessageBox>

// 实现热力图相关方法
void ScheduleDialog::showSegmentDialog()
{
	if (!heatmapSegmentDlg) {
		heatmapSegmentDlg = new HeatmapSegmentDialog(this);
		heatmapSegmentDlg->setStudentData(m_students);
		connect(heatmapSegmentDlg, &QDialog::accepted, this, [this]() {
			if (heatmapSegmentDlg) {
				m_segments = heatmapSegmentDlg->getSegments();
				m_heatmapType = 1; // 分段图
				updateSeatColors();
			}
		});
	}
	heatmapSegmentDlg->show();
	heatmapSegmentDlg->raise();
	heatmapSegmentDlg->activateWindow();
}

void ScheduleDialog::showGradientHeatmap()
{
	if (!heatmapViewDlg) {
		heatmapViewDlg = new HeatmapViewDialog(this);
		heatmapViewDlg->setStudentData(m_students);
		heatmapViewDlg->setHeatmapType(2); // 渐变热力图
	}
	heatmapViewDlg->show();
	heatmapViewDlg->raise();
	heatmapViewDlg->activateWindow();
}

void ScheduleDialog::setSegments(const QList<struct SegmentRange>& segments)
{
	m_segments = segments;
	m_heatmapType = 1;
	updateSeatColors();
}

void ScheduleDialog::updateSeatColors()
{
	if (!seatTable || m_students.isEmpty()) return;
	
	// 获取所有座位按钮
	QList<QPushButton*> seatButtons;
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 11; ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				seatButtons.append(btn);
			}
		}
	}
	
	if (m_heatmapType == 1) {
		// 分段图：根据分段区间设置颜色
		for (QPushButton* btn : seatButtons) {
			QString studentId = btn->property("studentId").toString();
			if (studentId.isEmpty()) continue;
			
			// 查找对应的学生
			StudentInfo* student = nullptr;
			for (auto& s : m_students) {
				if (s.id == studentId) {
					student = &s;
					break;
				}
			}
			
			if (!student) continue;
			
			// 根据成绩找到对应的分段
			QColor seatColor = QColor("#dc3545"); // 默认红色
			for (const auto& segment : m_segments) {
				if (student->score >= segment.minValue && student->score <= segment.maxValue) {
					seatColor = segment.color;
					break;
				}
			}
			
			// 设置按钮背景色
			btn->setStyleSheet(QString(
				"QPushButton { "
				"background-color: %1; "
				"color: white; "
				"border: 1px solid #ccc; "
				"border-radius: 4px; "
				"padding: 5px; "
				"font-size: 12px; "
				"}"
				"QPushButton:hover { "
				"background-color: %2; "
				"}"
			).arg(seatColor.name()).arg(seatColor.darker(120).name()));
		}
	} else if (m_heatmapType == 2) {
		// 渐变图：根据成绩计算渐变颜色
		// 找到最小和最大成绩
		double minScore = m_students[0].score;
		double maxScore = m_students[0].score;
		for (const auto& student : m_students) {
			if (student.score < minScore) minScore = student.score;
			if (student.score > maxScore) maxScore = student.score;
		}
		
		double range = maxScore - minScore;
		if (range == 0) range = 1; // 避免除零
		
		for (QPushButton* btn : seatButtons) {
			QString studentId = btn->property("studentId").toString();
			if (studentId.isEmpty()) continue;
			
			// 查找对应的学生
			StudentInfo* student = nullptr;
			for (auto& s : m_students) {
				if (s.id == studentId) {
					student = &s;
					break;
				}
			}
			
			if (!student) continue;
			
			// 计算归一化值 (0-1)
			double normalized = (student->score - minScore) / range;
			
			// 计算渐变颜色：蓝色(低) -> 绿色 -> 黄色 -> 红色(高)
			QColor seatColor;
			if (normalized < 0.33) {
				// 蓝色到绿色
				double t = normalized / 0.33;
				seatColor = QColor(
					int(0 + t * 0),
					int(0 + t * 255),
					int(255 - t * 0)
				);
			} else if (normalized < 0.66) {
				// 绿色到黄色
				double t = (normalized - 0.33) / 0.33;
				seatColor = QColor(
					int(0 + t * 255),
					255,
					int(255 - t * 255)
				);
			} else {
				// 黄色到红色
				double t = (normalized - 0.66) / 0.34;
				seatColor = QColor(
					255,
					int(255 - t * 255),
					0
				);
			}
			
			// 设置按钮背景色
			btn->setStyleSheet(QString(
				"QPushButton { "
				"background-color: %1; "
				"color: white; "
				"border: 1px solid #ccc; "
				"border-radius: 4px; "
				"padding: 5px; "
				"font-size: 12px; "
				"}"
				"QPushButton:hover { "
				"background-color: %2; "
				"}"
			).arg(seatColor.name()).arg(seatColor.darker(120).name()));
		}
	}
}

