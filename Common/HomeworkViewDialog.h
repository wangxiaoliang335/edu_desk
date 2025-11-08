#pragma once
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QDate>
#include <QMap>

class HomeworkViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HomeworkViewDialog(QWidget* parent = nullptr);
    
    // 设置日期
    void setDate(const QDate& date);
    
    // 设置作业内容
    void setHomeworkContent(const QMap<QString, QString>& content);

private slots:
    void onCloseClicked();

private:
    QLabel* dateLabel;
    QMap<QString, QLabel*> subjectLabels; // 科目 -> 作业内容标签
    QPushButton* btnClose;
};

