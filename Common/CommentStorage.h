#pragma once
#include <QString>
#include <QMap>
#include <QDebug>

// 全局存储注释信息（按 classId_examName_term_studentId_fieldName 作为键）
class CommentStorage {
public:
    // 新接口：按 classId + term + studentId + fieldName 存储（不区分 examName）
    static void saveComment(const QString& classId, const QString& term,
                            const QString& studentId, const QString& fieldName, const QString& comment) {
        QString key = QString("%1_%2_%3_%4").arg(classId).arg(term).arg(studentId).arg(fieldName);
        if (!comment.isEmpty()) {
            s_comments[key] = comment;
        } else {
            s_comments.remove(key);
        }
    }

    static QString getComment(const QString& classId, const QString& term,
                              const QString& studentId, const QString& fieldName) {
        QString key = QString("%1_%2_%3_%4").arg(classId).arg(term).arg(studentId).arg(fieldName);
        return s_comments.value(key, "");
    }

    static void clearComments(const QString& classId, const QString& term) {
        QString prefix = QString("%1_%2_").arg(classId).arg(term);
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

    // 兼容旧接口：examName 参数将被忽略（保留签名避免大范围改动）
	// 保存注释
	static void saveComment(const QString& classId, const QString& examName, const QString& term,
	                        const QString& studentId, const QString& fieldName, const QString& comment) {
        Q_UNUSED(examName);
        saveComment(classId, term, studentId, fieldName, comment);
	}
	
	// 获取注释
	static QString getComment(const QString& classId, const QString& examName, const QString& term,
	                          const QString& studentId, const QString& fieldName) {
        Q_UNUSED(examName);
        return getComment(classId, term, studentId, fieldName);
	}
	
	// 清除指定班级、考试、学期的所有注释
	static void clearComments(const QString& classId, const QString& examName, const QString& term) {
        Q_UNUSED(examName);
        clearComments(classId, term);
	}
	
private:
	static QMap<QString, QString> s_comments;
};

