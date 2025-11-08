#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QDateEdit>
#include <QMap>

class HomeworkEditDialog : public QDialog
{
    Q_OBJECT

signals:
    void homeworkPublished(const QDate& date, const QMap<QString, QString>& homeworkContent);

public:
    explicit HomeworkEditDialog(QWidget* parent = nullptr);
    
    // 设置当前日期
    void setDate(const QDate& date);
    
    // 设置作业内容（用于编辑已有作业）
    void setHomeworkContent(const QMap<QString, QString>& content);

private slots:
    void onPublishClicked();
    void onCloseClicked();

private:
    QDateEdit* dateEdit;
    QMap<QString, QTextEdit*> subjectEdits; // 科目 -> 作业内容编辑框
    QPushButton* btnPublish;
    QPushButton* btnClose;
};

