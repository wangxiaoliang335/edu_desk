#pragma once
#include <QString>
#include <QMap>
#include <QDebug>

// 全局存储 score_header_id（按 class_id + exam_name + term 作为键）
class ScoreHeaderIdStorage {
public:
	static void saveScoreHeaderId(const QString& classId, const QString& examName, const QString& term, int scoreHeaderId) {
		QString key = QString("%1_%2_%3").arg(classId).arg(examName).arg(term);
		s_scoreHeaderIds[key] = scoreHeaderId;
		qDebug() << "已保存 score_header_id 到全局存储，键:" << key << "，ID:" << scoreHeaderId;
	}
	
	static int getScoreHeaderId(const QString& classId, const QString& examName, const QString& term) {
		QString key = QString("%1_%2_%3").arg(classId).arg(examName).arg(term);
		return s_scoreHeaderIds.value(key, -1);
	}
	
	static bool hasScoreHeaderId(const QString& classId, const QString& examName, const QString& term) {
		QString key = QString("%1_%2_%3").arg(classId).arg(examName).arg(term);
		return s_scoreHeaderIds.contains(key);
	}
	
private:
	static QMap<QString, int> s_scoreHeaderIds;
};

