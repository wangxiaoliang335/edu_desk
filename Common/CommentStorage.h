#pragma once
#include <QString>
#include <QMap>
#include <QDebug>

// 全局存储注释信息（按 classId_examName_term_studentId_fieldName 作为键）
class CommentStorage {
public:
	// 保存注释
	static void saveComment(const QString& classId, const QString& examName, const QString& term,
	                        const QString& studentId, const QString& fieldName, const QString& comment) {
		QString key = QString("%1_%2_%3_%4_%5").arg(classId).arg(examName).arg(term).arg(studentId).arg(fieldName);
		if (!comment.isEmpty()) {
			s_comments[key] = comment;
		} else {
			s_comments.remove(key);
		}
	}
	
	// 获取注释
	static QString getComment(const QString& classId, const QString& examName, const QString& term,
	                          const QString& studentId, const QString& fieldName) {
		QString key = QString("%1_%2_%3_%4_%5").arg(classId).arg(examName).arg(term).arg(studentId).arg(fieldName);
		return s_comments.value(key, "");
	}
	
	// 清除指定班级、考试、学期的所有注释
	static void clearComments(const QString& classId, const QString& examName, const QString& term) {
		QString prefix = QString("%1_%2_%3_").arg(classId).arg(examName).arg(term);
		QList<QString> keysToRemove;
		for (auto it = s_comments.begin(); it != s_comments.end(); ++it) {
			if (it.key().startsWith(prefix)) {
				keysToRemove.append(it.key());
			}
		}
		for (const QString& key : keysToRemove) {
			s_comments.remove(key);
		}
	}
	
private:
	static QMap<QString, QString> s_comments;
};

